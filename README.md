# ESP32 Agenda OLED

Proyecto nuevo para `ESP32 + SSD1306 128x64` que muestra tu agenda de hoy en pantalla.

La interfaz esta pensada para que:

- arriba se vea la fecha actual
- en el centro aparezca la linea de `AHORA`
- el listado quede anclado a la hora actual
- la reunion en curso o la siguiente quede cerca del centro
- cada cita muestre `hora + titulo`

## Enfoque recomendado

Para un calendario privado de Google, en un ESP32 conviene **no** hacer OAuth completo dentro del microcontrolador. En este proyecto el flujo es:

1. `Google Apps Script` lee tu Google Calendar
2. ese script expone un `JSON` simple por HTTPS
3. el `ESP32` consulta ese endpoint cada pocos minutos
4. la `OLED` renderiza la agenda del dia

Esto es mucho mas estable y sencillo de mantener.

## Navegacion con botones

- `GPIO 5`: avanza hacia abajo entre las reuniones del listado
- `GPIO 6`: abre el detalle de la reunion enfocada
- `GPIO 7`: vuelve desde detalle al listado del dia

En la vista principal, la reunion enfocada arranca en la que queda centrada por la hora actual y luego puedes moverla manualmente con `GPIO 5`.

## Estructura

```text
esp32_calendar_oled/
|-- esp32_calendar_oled.ino
|-- README.md
`-- src/
    |-- App.cpp
    |-- App.h
    |-- CalendarEvent.h
    |-- CalendarService.cpp
    |-- CalendarService.h
    |-- Config.h
    |-- DisplayManager.cpp
    |-- DisplayManager.h
    `-- Secrets.example.h
```

## Librerias para Arduino IDE

Instala estas librerias:

- `U8g2`
- `ArduinoJson`

La libreria `WiFi` y `HTTPClient` ya vienen con el core del ESP32.

## Conexion SSD1306

Configuracion I2C tipica:

- `VCC` -> `3V3`
- `GND` -> `GND`
- `SCL` -> `GPIO 22`
- `SDA` -> `GPIO 21`

Si tu placa usa otros pines I2C, ajustalo antes de iniciar `Wire`.

## Paso 1: crear tu archivo de secretos

Duplica:

- [`src/Secrets.example.h`](/Users/mauro/Documents/Playground/esp32_calendar_oled/src/Secrets.example.h)

como:

- `src/Secrets.h`

y completa:

- tu `WiFi`
- la `URL` del Web App de Apps Script
- un `token` secreto compartido

Tambien dejé un [`src/Secrets.h`](/Users/mauro/Documents/Playground/esp32_calendar_oled/src/Secrets.h) con placeholders para que el proyecto abra completo desde el primer momento. Solo reemplaza esos valores.

## Paso 2: crear el endpoint de Google Apps Script

En [script.google.com](https://script.google.com/) crea un proyecto nuevo y pega esto:

```javascript
const TOKEN = 'CAMBIA_ESTE_TOKEN';
const CALENDAR_ID = 'primary';

function doGet(e) {
  const token = e.parameter.token || '';
  if (token !== TOKEN) {
    return jsonOutput({ error: 'unauthorized' }, 401);
  }

  const calendar = CalendarApp.getCalendarById(CALENDAR_ID);
  const now = new Date();
  const start = new Date(now);
  start.setHours(0, 0, 0, 0);

  const end = new Date(now);
  end.setHours(23, 59, 59, 999);

  const events = calendar.getEvents(start, end).map((event) => ({
    title: event.getTitle(),
    description: event.getDescription(),
    startEpoch: Math.floor(event.getStartTime().getTime() / 1000),
    endEpoch: Math.floor(event.getEndTime().getTime() / 1000),
    allDay: event.isAllDayEvent(),
  }));

  return jsonOutput({
    date: Utilities.formatDate(now, Session.getScriptTimeZone(), 'yyyy-MM-dd'),
    timezone: Session.getScriptTimeZone(),
    events,
  });
}

function jsonOutput(obj) {
  return ContentService.createTextOutput(JSON.stringify(obj)).setMimeType(
    ContentService.MimeType.JSON
  );
}
```

## Paso 3: desplegar el script

1. En Apps Script elige `Deploy`
2. Luego `New deployment`
3. Tipo `Web app`
4. Ejecutar como `Me`
5. Acceso `Anyone`
6. Copia la URL del deployment y pegala en `Secrets.h`

Usa el mismo valor del `TOKEN` en:

- el script
- `Secrets::CALENDAR_TOKEN`

## Paso 4: compilar en Arduino IDE

1. Abre [`esp32_calendar_oled.ino`](/Users/mauro/Documents/Playground/esp32_calendar_oled/esp32_calendar_oled.ino)
2. Selecciona tu placa ESP32
3. Verifica que exista `src/Secrets.h`
4. Compila y sube

## Comportamiento de la pantalla

- La linea central marca `AHORA`
- Si hay una reunion en curso, esa reunion se vuelve el foco
- Si no hay una reunion en curso, se enfoca la siguiente
- Las reuniones anteriores quedan arriba
- Las futuras quedan abajo
- Si falla una consulta, se mantiene la ultima agenda cargada

## Ajustes rapidos utiles

Puedes cambiar estos valores en [`src/Config.h`](/Users/mauro/Documents/Playground/esp32_calendar_oled/src/Config.h):

- `CALENDAR_REFRESH_MS`
- `TZ_INFO`
- `OLED_ADDR`
- `MAX_EVENTS`

## Nota importante sobre "calendario de Gmail"

Tecnicamente las reuniones viven en `Google Calendar`, aunque uses la misma cuenta de Gmail. Este proyecto lee tu agenda de Google Calendar asociada a esa cuenta.
