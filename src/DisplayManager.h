#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "CalendarEvent.h"

class DisplayManager {
 public:
  DisplayManager();
  bool begin();
  void renderBoot(const char* line1, const char* line2);
  void renderSchedule(const CalendarEvent* events, size_t count, time_t nowEpoch,
                      int selectedEventIndex, bool blink, bool staleData);
  void renderEventDetail(const CalendarEvent& event, uint8_t scrollOffset, bool staleData);
  int currentAnchorIndex(const CalendarEvent* events, size_t count, time_t nowEpoch) const;
  uint8_t detailMaxScroll(const CalendarEvent& event, uint8_t maxCharsPerLine,
                          uint8_t visibleLines) const;

 private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C display_;

  void renderHeader(time_t nowEpoch, bool staleData);
  void renderDetailHeader(const CalendarEvent& event, bool staleData);
  void renderNowLine(time_t nowEpoch, bool blink);
  int findAnchorIndex(const CalendarEvent* events, size_t count, time_t nowEpoch) const;
  void drawEventRow(int rowY, const CalendarEvent& event, time_t nowEpoch, bool highlight);
  String formatDate(time_t epoch) const;
  String formatTime(time_t epoch) const;
  String formatDayNameEs(time_t epoch) const;
  String trimTitle(const char* title, size_t maxChars) const;
  uint8_t drawWrappedText(int startX, int startY, const char* text, uint8_t maxCharsPerLine,
                          uint8_t maxLines, uint8_t scrollOffset, bool drawIndicator);
  uint8_t countWrappedLines(const char* text, uint8_t maxCharsPerLine) const;
  void drawText(int x, int y, const String& text);
};
