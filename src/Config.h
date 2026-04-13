#pragma once

#include <Arduino.h>

namespace Config {
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr int8_t OLED_RESET = -1;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t BUTTON_NEXT_PIN = 5;
constexpr uint8_t BUTTON_BACK_PIN = 7;
constexpr uint8_t BUTTON_DETAIL_PIN = 6;

constexpr char NTP_SERVER[] = "pool.ntp.org";
constexpr char TZ_INFO[] = "CLT4CLST,M9.1.6/24,M4.1.6/24";

constexpr unsigned long WIFI_RETRY_MS = 10000;
constexpr unsigned long CLOCK_SYNC_RETRY_MS = 5000;
constexpr unsigned long CALENDAR_REFRESH_MS = 15UL * 1000UL;
constexpr unsigned long RENDER_BLINK_MS = 600;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 140;
constexpr unsigned long NAVIGATION_IDLE_BEFORE_REFRESH_MS = 2500;

constexpr size_t MAX_EVENTS = 12;
constexpr size_t TITLE_MAX_LEN = 48;
constexpr size_t CREATOR_MAX_LEN = 48;
constexpr size_t DESCRIPTION_MAX_LEN = 320;
constexpr uint16_t HTTP_TIMEOUT_MS = 5000;

constexpr uint8_t HEADER_HEIGHT = 13;
constexpr uint8_t ROW_HEIGHT = 14;
constexpr uint8_t NOW_LINE_Y = 36;
constexpr uint8_t MAX_VISIBLE_ROWS = 5;
}  // namespace Config
