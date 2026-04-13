#include "src/App.h"

App app;

void setup() {
  Serial.begin(115200);
  delay(200);
  app.begin();
}

void loop() {
  app.update();
  app.render();
  delay(33);
}
