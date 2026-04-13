#include "CalendarService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "Config.h"
#include "Secrets.h"

String CalendarService::buildUrl(int dayOffset) const {
  String url = Secrets::CALENDAR_ENDPOINT;
  if (url.indexOf('?') >= 0) {
    url += "&token=";
  } else {
    url += "?token=";
  }
  url += Secrets::CALENDAR_TOKEN;
  url += "&dayOffset=";
  url += String(dayOffset);
  return url;
}

bool CalendarService::fetchDayEvents(int dayOffset, CalendarEvent* events, size_t maxEvents,
                                       size_t& outCount, String& outError) {
  outCount = 0;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(Config::HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setReuse(false);

  const char* headerKeys[] = {"Location"};
  http.collectHeaders(headerKeys, 1);

  const String url = buildUrl(dayOffset);
  if (!http.begin(client, url)) {
    outError = "No se pudo abrir HTTPS";
    return false;
  }

  const int httpCode = http.GET();
  if (httpCode <= 0) {
    outError = "GET fallo: " + http.errorToString(httpCode);
    http.end();
    return false;
  }

  if (httpCode != HTTP_CODE_OK) {
    outError = "HTTP " + String(httpCode);

    const String location = http.header("Location");
    if (location.length() > 0) {
      outError += " -> ";
      outError += location.substring(0, 18);
    }

    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();
  return parseResponse(payload, events, maxEvents, outCount, outError);
}

bool CalendarService::parseResponse(const String& payload, CalendarEvent* events,
                                    size_t maxEvents, size_t& outCount,
                                    String& outError) {
  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    outError = "JSON invalido";
    return false;
  }

  JsonArray jsonEvents = doc["events"].as<JsonArray>();
  if (jsonEvents.isNull()) {
    outError = "Sin lista de eventos";
    return false;
  }

  for (JsonVariant item : jsonEvents) {
    if (outCount >= maxEvents) {
      break;
    }

    CalendarEvent& event = events[outCount];
    copyTitle(String(item["title"] | "Sin titulo"), event);
    String creator = String(item["dataowner"] | "");
    if (creator.length() == 0) {
      creator = String(item["organizer"] | "");
    }
    copyCreator(creator, event);
    String details = String(item["details"] | "");
    if (details.length() == 0) {
      details = String(item["description"] | "");
    }
    copyDescription(details, event);
    event.startEpoch = item["startEpoch"] | 0;
    event.endEpoch = item["endEpoch"] | 0;
    event.allDay = item["allDay"] | false;
    event.valid = event.startEpoch > 0;

    if (event.valid) {
      ++outCount;
    }
  }

  outError = "";
  return true;
}

void CalendarService::copyTitle(const String& source, CalendarEvent& event) const {
  source.substring(0, Config::TITLE_MAX_LEN - 1)
      .toCharArray(event.title, Config::TITLE_MAX_LEN);
}

void CalendarService::copyCreator(const String& source, CalendarEvent& event) const {
  source.substring(0, Config::CREATOR_MAX_LEN - 1)
      .toCharArray(event.creator, Config::CREATOR_MAX_LEN);
}

void CalendarService::copyDescription(const String& source, CalendarEvent& event) const {
  String normalized = source;
  normalized.replace("\r", " ");
  normalized.replace("\n", " ");
  normalized.trim();
  normalized.substring(0, Config::DESCRIPTION_MAX_LEN - 1)
      .toCharArray(event.description, Config::DESCRIPTION_MAX_LEN);
}
