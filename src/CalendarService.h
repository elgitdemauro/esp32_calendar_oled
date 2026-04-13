#pragma once

#include <Arduino.h>

#include "CalendarEvent.h"

class CalendarService {
 public:
  bool fetchDayEvents(int dayOffset, CalendarEvent* events, size_t maxEvents, size_t& outCount,
                        String& outError);

 private:
  String buildUrl(int dayOffset) const;
  bool parseResponse(const String& payload, CalendarEvent* events, size_t maxEvents,
                     size_t& outCount, String& outError);
  void copyTitle(const String& source, CalendarEvent& event) const;
  void copyCreator(const String& source, CalendarEvent& event) const;
  void copyDescription(const String& source, CalendarEvent& event) const;
};
