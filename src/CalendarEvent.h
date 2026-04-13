#pragma once

#include <Arduino.h>

#include "Config.h"

struct CalendarEvent {
  char title[Config::TITLE_MAX_LEN];
  char creator[Config::CREATOR_MAX_LEN];
  char description[Config::DESCRIPTION_MAX_LEN];
  time_t startEpoch = 0;
  time_t endEpoch = 0;
  bool allDay = false;
  bool valid = false;
};
