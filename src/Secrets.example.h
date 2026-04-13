#pragma once

namespace Secrets {
constexpr char WIFI_SSID[] = "TU_WIFI";
constexpr char WIFI_PASSWORD[] = "TU_PASSWORD";

// URL del despliegue Web App de Google Apps Script.
constexpr char CALENDAR_ENDPOINT[] =
    "https://script.google.com/macros/s/TU_DEPLOYMENT_ID/exec";

// Debe coincidir con el valor TOKEN del script.
constexpr char CALENDAR_TOKEN[] = "CAMBIA_ESTE_TOKEN";
}  // namespace Secrets
