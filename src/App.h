#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "CalendarEvent.h"
#include "CalendarService.h"
#include "DisplayManager.h"

class App {
 public:
  void begin();
  void update();
  void render();

 private:
  enum class ViewMode { kSchedule, kDetail };

  DisplayManager display_;
  CalendarService calendarService_;
  CalendarEvent events_[Config::MAX_EVENTS];
  CalendarEvent fetchedEvents_[Config::MAX_EVENTS];

  size_t eventCount_ = 0;
  size_t fetchedEventCount_ = 0;
  int fetchedDayOffset_ = 0;
  unsigned long lastWiFiAttemptMs_ = 0;
  unsigned long lastClockSyncAttemptMs_ = 0;
  unsigned long lastCalendarRequestMs_ = 0;
  unsigned long lastBlinkToggleMs_ = 0;
  unsigned long lastInteractionMs_ = 0;

  bool displayReady_ = false;
  bool clockReady_ = false;
  bool calendarEverLoaded_ = false;
  bool staleData_ = false;
  bool blinkState_ = true;
  String statusLine_ = "Iniciando";
  String fetchError_;
  ViewMode viewMode_ = ViewMode::kSchedule;
  int selectedEventIndex_ = -1;
  uint8_t detailScrollOffset_ = 0;
  bool selectionLocked_ = false;
  bool nextPressed_ = false;
  bool detailPressed_ = false;
  bool backPressed_ = false;
  volatile bool calendarFetchInProgress_ = false;
  volatile bool calendarDataReady_ = false;
  volatile bool calendarFetchFailed_ = false;
  unsigned long lastNextPressMs_ = 0;
  unsigned long lastDetailPressMs_ = 0;
  unsigned long lastBackPressMs_ = 0;
  TaskHandle_t calendarTaskHandle_ = nullptr;
  SemaphoreHandle_t calendarMutex_ = nullptr;

  static void calendarTaskEntry(void* parameter);
  void calendarTaskLoop();
  void ensureWiFi();
  void ensureClock();
  void refreshCalendarIfNeeded(bool force = false);
  void applyFetchedCalendarData();
  bool isWiFiReady() const;
  bool isClockReady() const;
  time_t currentEpoch() const;
  void updateButtons();
  bool consumeButtonPress(uint8_t pin, bool& wasPressed, unsigned long& lastPressMs);
  void syncSelectedEvent(bool preserveSelection = true);
  void markInteraction();
};
