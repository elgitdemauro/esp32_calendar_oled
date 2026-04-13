#include "App.h"

#include <WiFi.h>
#include <cstring>
#include <time.h>

#include "Config.h"
#include "Secrets.h"

void App::begin() {
  displayReady_ = display_.begin();
  if (!displayReady_) {
    return;
  }

  pinMode(Config::BUTTON_NEXT_PIN, INPUT_PULLUP);
  pinMode(Config::BUTTON_DETAIL_PIN, INPUT_PULLUP);
  pinMode(Config::BUTTON_BACK_PIN, INPUT_PULLUP);

  display_.renderBoot("Conectando WiFi", "Esperando reloj");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  calendarMutex_ = xSemaphoreCreateMutex();
  if (calendarMutex_ != nullptr) {
    xTaskCreatePinnedToCore(calendarTaskEntry, "calendar_fetch", 12288, this, 1,
                            &calendarTaskHandle_, 0);
  }
}

void App::update() {
  if (!displayReady_) {
    return;
  }

  const unsigned long nowMs = millis();
  if (nowMs - lastBlinkToggleMs_ >= Config::RENDER_BLINK_MS) {
    blinkState_ = !blinkState_;
    lastBlinkToggleMs_ = nowMs;
  }

  ensureWiFi();
  ensureClock();
  updateButtons();
  applyFetchedCalendarData();
  refreshCalendarIfNeeded();
  syncSelectedEvent();
}

void App::render() {
  if (!displayReady_) {
    return;
  }

  if (!isWiFiReady()) {
    display_.renderBoot("Conectando WiFi", WiFi.SSID().c_str());
    return;
  }

  if (!isClockReady()) {
    display_.renderBoot("WiFi listo", "Sin hora NTP");
    return;
  }

  if (viewMode_ == ViewMode::kDetail && selectedEventIndex_ >= 0 &&
      selectedEventIndex_ < static_cast<int>(eventCount_)) {
    display_.renderEventDetail(events_[selectedEventIndex_], detailScrollOffset_, staleData_);
    return;
  }

  display_.renderSchedule(events_, eventCount_, currentEpoch(), selectedEventIndex_,
                          blinkState_, staleData_);
}

void App::ensureWiFi() {
  if (isWiFiReady()) {
    return;
  }

  const unsigned long nowMs = millis();
  if (nowMs - lastWiFiAttemptMs_ < Config::WIFI_RETRY_MS) {
    return;
  }

  lastWiFiAttemptMs_ = nowMs;
  statusLine_ = "Conectando WiFi";
  WiFi.begin(Secrets::WIFI_SSID, Secrets::WIFI_PASSWORD);
}

void App::ensureClock() {
  if (!isWiFiReady() || clockReady_) {
    return;
  }

  const unsigned long nowMs = millis();
  if (nowMs - lastClockSyncAttemptMs_ < Config::CLOCK_SYNC_RETRY_MS) {
    return;
  }

  lastClockSyncAttemptMs_ = nowMs;
  configTzTime(Config::TZ_INFO, Config::NTP_SERVER);

  time_t now = time(nullptr);
  if (now > 1700000000) {
    clockReady_ = true;
    statusLine_ = "Hora sincronizada";
    refreshCalendarIfNeeded(true);
  } else {
    statusLine_ = "Esperando NTP";
  }
}

void App::refreshCalendarIfNeeded(bool force) {
  if (!isWiFiReady() || !isClockReady()) {
    return;
  }

  if (calendarFetchInProgress_) {
    return;
  }

  if (!force) {
    if (viewMode_ == ViewMode::kDetail) {
      return;
    }

    const unsigned long nowMs = millis();
    if (nowMs - lastInteractionMs_ < Config::NAVIGATION_IDLE_BEFORE_REFRESH_MS) {
      return;
    }
  }

  const unsigned long nowMs = millis();
  if (!force && nowMs - lastCalendarRequestMs_ < Config::CALENDAR_REFRESH_MS) {
    return;
  }

  lastCalendarRequestMs_ = nowMs;
  statusLine_ = "Actualizando";
  calendarFetchInProgress_ = true;
}

bool App::isWiFiReady() const { return WiFi.status() == WL_CONNECTED; }

bool App::isClockReady() const { return clockReady_; }

time_t App::currentEpoch() const { return time(nullptr); }

void App::updateButtons() {
  if (consumeButtonPress(Config::BUTTON_NEXT_PIN, nextPressed_, lastNextPressMs_)) {
    markInteraction();
    if (viewMode_ == ViewMode::kDetail && selectedEventIndex_ >= 0 &&
        selectedEventIndex_ < static_cast<int>(eventCount_)) {
      const uint8_t maxScroll =
          display_.detailMaxScroll(events_[selectedEventIndex_], 21, 3);
      if (detailScrollOffset_ < maxScroll) {
        ++detailScrollOffset_;
      }
    } else if (viewMode_ == ViewMode::kSchedule && eventCount_ > 0) {
      if (selectedEventIndex_ < 0) {
        selectedEventIndex_ = 0;
      } else if (selectedEventIndex_ < static_cast<int>(eventCount_ - 1)) {
        ++selectedEventIndex_;
      } else {
        selectedEventIndex_ = 0;
      }
      selectionLocked_ = true;
    }
  }

  if (consumeButtonPress(Config::BUTTON_DETAIL_PIN, detailPressed_, lastDetailPressMs_)) {
    markInteraction();
    if (viewMode_ == ViewMode::kSchedule && selectedEventIndex_ >= 0 &&
        selectedEventIndex_ < static_cast<int>(eventCount_)) {
      detailScrollOffset_ = 0;
      viewMode_ = ViewMode::kDetail;
    }
  }

  if (consumeButtonPress(Config::BUTTON_BACK_PIN, backPressed_, lastBackPressMs_)) {
    markInteraction();
    if (viewMode_ == ViewMode::kDetail) {
      detailScrollOffset_ = 0;
      viewMode_ = ViewMode::kSchedule;
    }
  }
}

bool App::consumeButtonPress(uint8_t pin, bool& wasPressed, unsigned long& lastPressMs) {
  const bool isPressed = digitalRead(pin) == LOW;
  const unsigned long nowMs = millis();

  if (isPressed && !wasPressed && nowMs - lastPressMs >= Config::BUTTON_DEBOUNCE_MS) {
    wasPressed = true;
    lastPressMs = nowMs;
    return true;
  }

  if (!isPressed) {
    wasPressed = false;
  }

  return false;
}

void App::syncSelectedEvent(bool preserveSelection) {
  if (eventCount_ == 0) {
    selectedEventIndex_ = -1;
    detailScrollOffset_ = 0;
    selectionLocked_ = false;
    if (viewMode_ == ViewMode::kDetail) {
      viewMode_ = ViewMode::kSchedule;
    }
    return;
  }

  if (preserveSelection && selectedEventIndex_ >= 0 &&
      selectedEventIndex_ < static_cast<int>(eventCount_)) {
    return;
  }

  if (viewMode_ == ViewMode::kSchedule) {
    selectedEventIndex_ = display_.currentAnchorIndex(events_, eventCount_, currentEpoch());
    selectionLocked_ = false;
    return;
  }

  if (selectedEventIndex_ >= static_cast<int>(eventCount_)) {
    selectedEventIndex_ = static_cast<int>(eventCount_ - 1);
    detailScrollOffset_ = 0;
  }
}

void App::markInteraction() { lastInteractionMs_ = millis(); }

void App::applyFetchedCalendarData() {
  if (!calendarDataReady_ && !calendarFetchFailed_) {
    return;
  }

  if (calendarMutex_ == nullptr) {
    return;
  }

  if (xSemaphoreTake(calendarMutex_, 0) != pdTRUE) {
    return;
  }

  if (calendarDataReady_) {
    memcpy(events_, fetchedEvents_, sizeof(events_));
    eventCount_ = fetchedEventCount_;
    staleData_ = false;
    statusLine_ = eventCount_ > 0 ? "" : "Sin reuniones hoy";
    calendarDataReady_ = false;
    syncSelectedEvent(selectionLocked_);
  } else if (calendarFetchFailed_) {
    staleData_ = true;
    statusLine_ = fetchError_.length() > 0 ? fetchError_ : "Error calendario";
  }

  calendarFetchFailed_ = false;
  calendarFetchInProgress_ = false;
  xSemaphoreGive(calendarMutex_);
}

void App::calendarTaskEntry(void* parameter) {
  static_cast<App*>(parameter)->calendarTaskLoop();
}

void App::calendarTaskLoop() {
  for (;;) {
    if (!calendarFetchInProgress_) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    CalendarEvent localEvents[Config::MAX_EVENTS];
    size_t localCount = 0;
    String localError;
    const bool ok =
        calendarService_.fetchDayEvents(0, localEvents, Config::MAX_EVENTS, localCount, localError);

    if (calendarMutex_ != nullptr && xSemaphoreTake(calendarMutex_, portMAX_DELAY) == pdTRUE) {
      if (ok) {
        memcpy(fetchedEvents_, localEvents, sizeof(localEvents));
        fetchedEventCount_ = localCount;
        calendarDataReady_ = true;
        calendarFetchFailed_ = false;
        fetchError_ = "";
      } else {
        calendarDataReady_ = false;
        calendarFetchFailed_ = true;
        fetchError_ = localError;
      }
      xSemaphoreGive(calendarMutex_);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
