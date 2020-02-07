#include <Arduino.h>

const int LED = 5;

void setup() {
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, LOW);
  delay(500); // ms
  digitalWrite(LED, HIGH);
  delay(500); // ms
}
