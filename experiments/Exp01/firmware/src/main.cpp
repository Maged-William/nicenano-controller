#include <Arduino.h>

#define LED PIN_015

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
}

void loop() {
  digitalWrite(LED, LOW);
  delay(200);
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
  delay(200);
  digitalWrite(LED, HIGH);
  delay(2000);
}
