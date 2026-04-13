#include "DisplayManager.h"

#include <Wire.h>

#include "Config.h"

DisplayManager::DisplayManager() : display_(U8G2_R0, U8X8_PIN_NONE) {}

bool DisplayManager::begin() {
  display_.setI2CAddress(Config::OLED_ADDR << 1);
  display_.begin();
  display_.setFont(u8g2_font_7x13_te);
  display_.setFontMode(1);
  display_.setDrawColor(1);
  display_.setFontPosTop();
  display_.clearBuffer();
  display_.sendBuffer();
  return true;
}

void DisplayManager::renderBoot(const char* line1, const char* line2) {
  display_.clearBuffer();
  drawText(0, 4, "Agenda OLED");
  // display_.drawHLine(0, 18, 128);
  drawText(0, 20, String(line1));
  drawText(0, 30, String(line2));
  display_.sendBuffer();
}

void DisplayManager::renderSchedule(const CalendarEvent* events, size_t count,
                                    time_t nowEpoch, int selectedEventIndex, bool blink,
                                    bool staleData, bool headerView) {
  display_.clearBuffer();
  renderHeader(nowEpoch, staleData, headerView);
  renderNowLine(nowEpoch, blink);

  if (count == 0) {
    drawText(10, 22, "No hay reuniones");
    drawText(18, 31, "agendadas hoy");
    display_.sendBuffer();
    return;
  }

  int selectedIndex;
  int anchorIndex;
  if (headerView) {
    anchorIndex = 2;
    selectedIndex = -1;
  } else {
    selectedIndex = selectedEventIndex;
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(count)) {
      selectedIndex = findAnchorIndex(events, count, nowEpoch);
    }
    anchorIndex = max(selectedIndex, 2);
  }

  const int centerRow = 2;
  for (int offset = -2; offset <= 2; ++offset) {
    const int eventIndex = anchorIndex + offset;
    if (eventIndex < 0 || eventIndex >= static_cast<int>(count)) {
      continue;
    }

    const int slot = centerRow + offset;
    const int rowY = Config::HEADER_HEIGHT + (slot * Config::ROW_HEIGHT) + 4;
    drawEventRow(rowY, events[eventIndex], nowEpoch, !headerView && eventIndex == selectedIndex);
  }

  display_.sendBuffer();
}

void DisplayManager::renderEventDetail(const CalendarEvent& event, uint8_t scrollOffset,
                                       bool staleData) {
  display_.clearBuffer();
  renderDetailHeader(event, staleData);
  // El string (" ") es donde va el Por: 
  drawText(0, 22, String("") +
                     (event.creator[0] != '\0' ? trimTitle(event.creator, 17)
                                               : String("Sin creador")));
  // display_.drawHLine(0, 27, 128);
  drawWrappedText(0, 35, event.description[0] != '\0' ? event.description : "Sin descripción",
                  21, 3, scrollOffset, true);
  display_.sendBuffer();
}

void DisplayManager::renderHeader(time_t nowEpoch, bool staleData, bool highlight) {
  struct tm timeInfo;
  localtime_r(&nowEpoch, &timeInfo);

  char dateBuffer[6];
  snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d", timeInfo.tm_mday, timeInfo.tm_mon + 1);

  if (highlight) {
    display_.drawBox(0, 0, 128, Config::HEADER_HEIGHT);
    display_.setDrawColor(0);
  }

  drawText(0, 0, String(dateBuffer));
  drawText(39, 0, formatDayNameEs(nowEpoch));
  if (staleData) {
    drawText(92, 0, "CACHE");
  }

  if (highlight) {
    display_.setDrawColor(1);
  }
}

void DisplayManager::renderDetailHeader(const CalendarEvent& event, bool staleData) {
  drawText(0, 0, trimTitle(event.title, 21));
  if (staleData) {
    drawText(92, 0, "CACHE");
  }

  String timeRange;
  if (event.allDay) {
    timeRange = "Todo el día";
  } else {
    timeRange = formatTime(event.startEpoch) + " - " + formatTime(event.endEpoch);
  }
  drawText(0, 9, timeRange);
}

void DisplayManager::renderNowLine(time_t nowEpoch, bool blink) {
  const int y = Config::NOW_LINE_Y;
  display_.setDrawColor(0);
  display_.drawBox(35, y - 4, 58, 11);
  display_.setDrawColor(1);

  String label = "AHORA ";
  if (blink) {
    label += formatTime(nowEpoch);
  } else {
    String timeText = formatTime(nowEpoch);
    for (size_t i = 0; i < timeText.length(); ++i) {
      label += (i == 2 ? ' ' : timeText[i]);
    }
  }

  display_.setDrawColor(0);
  drawText(40, y - 3, label);
  display_.setDrawColor(1);
}

int DisplayManager::currentAnchorIndex(const CalendarEvent* events, size_t count,
                                       time_t nowEpoch) const {
  return findAnchorIndex(events, count, nowEpoch);
}

int DisplayManager::findAnchorIndex(const CalendarEvent* events, size_t count,
                                    time_t nowEpoch) const {
  if (count == 0) {
    return 0;
  }

  for (size_t i = 0; i < count; ++i) {
    const CalendarEvent& event = events[i];
    if (nowEpoch >= event.startEpoch && nowEpoch < event.endEpoch) {
      return static_cast<int>(i);
    }
    if (event.startEpoch >= nowEpoch) {
      return static_cast<int>(i);
    }
  }

  return static_cast<int>(count - 1);
}

void DisplayManager::drawEventRow(int rowY, const CalendarEvent& event, time_t nowEpoch,
                                  bool highlight) {
  const bool isCurrent = nowEpoch >= event.startEpoch && nowEpoch < event.endEpoch;
  const bool isPast = event.endEpoch < nowEpoch;

  if (highlight) {
    display_.drawBox(0, rowY - 1, 128, 13);
    display_.setDrawColor(0);
  }

  drawText(0, rowY, event.allDay ? String("Todo") : formatTime(event.startEpoch));
  drawText(38, rowY, trimTitle(event.title, 15));

  if (isCurrent) {
    drawText(116, rowY, ">");
  } else if (isPast) {
    drawText(116, rowY, ".");
  }

  display_.setDrawColor(1);
}

String DisplayManager::formatDate(time_t epoch) const {
  struct tm timeInfo;
  localtime_r(&epoch, &timeInfo);

  char buffer[40];
  snprintf(buffer, sizeof(buffer), "%02d/%02d %s", timeInfo.tm_mday, timeInfo.tm_mon + 1,
           formatDayNameEs(epoch).c_str());
  return String(buffer);
}

String DisplayManager::formatTime(time_t epoch) const {
  struct tm timeInfo;
  localtime_r(&epoch, &timeInfo);

  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", &timeInfo);
  return String(buffer);
}

String DisplayManager::formatDayNameEs(time_t epoch) const {
  static const char* kDays[] = {"Domingo", "Lunes", "Martes", "Miércoles",
                                "Jueves", "Viernes", "Sábado"};

  struct tm timeInfo;
  localtime_r(&epoch, &timeInfo);
  if (timeInfo.tm_wday < 0 || timeInfo.tm_wday > 6) {
    return String("día");
  }
  return String(kDays[timeInfo.tm_wday]);
}

String DisplayManager::trimTitle(const char* title, size_t maxChars) const {
  String text(title);
  if (text.length() <= maxChars) {
    return text;
  }
  return text.substring(0, maxChars - 1) + ".";
}

uint8_t DisplayManager::detailMaxScroll(const CalendarEvent& event, uint8_t maxCharsPerLine,
                                        uint8_t visibleLines) const {
  const char* text = event.description[0] != '\0' ? event.description : "Sin descripción";
  const uint8_t totalLines = countWrappedLines(text, maxCharsPerLine);
  return totalLines > visibleLines ? totalLines - visibleLines : 0;
}

uint8_t DisplayManager::countWrappedLines(const char* text, uint8_t maxCharsPerLine) const {
  String source(text);
  source.trim();
  if (source.length() == 0) {
    return 1;
  }

  int cursor = 0;
  uint8_t totalLines = 0;
  while (cursor < source.length()) {
    int end = min(cursor + static_cast<int>(maxCharsPerLine), static_cast<int>(source.length()));
    int breakPos = end;

    if (end < source.length()) {
      while (breakPos > cursor && source[breakPos] != ' ') {
        --breakPos;
      }
      if (breakPos == cursor) {
        breakPos = end;
      }
    }

    ++totalLines;
    cursor = breakPos;
    while (cursor < source.length() && source[cursor] == ' ') {
      ++cursor;
    }
  }

  return totalLines == 0 ? 1 : totalLines;
}

uint8_t DisplayManager::drawWrappedText(int startX, int startY, const char* text,
                                        uint8_t maxCharsPerLine, uint8_t maxLines,
                                        uint8_t scrollOffset, bool drawIndicator) {
  String source(text);
  source.trim();
  if (source.length() == 0) {
    source = " ";
  }

  const uint8_t totalLines = countWrappedLines(source.c_str(), maxCharsPerLine);
  const uint8_t maxScroll = totalLines > maxLines ? totalLines - maxLines : 0;
  const uint8_t safeScroll = min(scrollOffset, maxScroll);

  int cursor = 0;
  uint8_t producedLine = 0;
  uint8_t drawnLine = 0;

  while (cursor < source.length() && drawnLine < maxLines) {
    int end = min(cursor + static_cast<int>(maxCharsPerLine), static_cast<int>(source.length()));
    int breakPos = end;

    if (end < source.length()) {
      while (breakPos > cursor && source[breakPos] != ' ') {
        --breakPos;
      }
      if (breakPos == cursor) {
        breakPos = end;
      }
    }

    String chunk = source.substring(cursor, breakPos);
    chunk.trim();
    if (producedLine >= safeScroll) {
      if (drawnLine == maxLines - 1 && producedLine < totalLines - 1 && chunk.length() > 0) {
        if (chunk.length() >= maxCharsPerLine) {
          chunk = chunk.substring(0, maxCharsPerLine - 1);
        }
        chunk += ".";
      }
      drawText(startX, startY + (drawnLine * 10), chunk);
      ++drawnLine;
    }

    ++producedLine;
    cursor = breakPos;
    while (cursor < source.length() && source[cursor] == ' ') {
      ++cursor;
    }
  }

  if (drawIndicator && safeScroll < maxScroll) {
    display_.drawTriangle(121, 57, 127, 57, 124, 62);
  }

  return maxScroll;
}

void DisplayManager::drawText(int x, int y, const String& text) {
  display_.drawUTF8(x, y, text.c_str());
}
