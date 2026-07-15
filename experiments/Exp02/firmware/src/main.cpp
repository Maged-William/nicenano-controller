#include <Arduino.h>
#include <Wire.h>

#define LED PIN_015
#define CONV_REG  0x00
#define CONFIG_REG 0x01

static uint8_t adc_addr = 0;

static uint16_t readRegister(uint8_t reg) {
  Wire.beginTransmission(adc_addr);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(adc_addr, (uint8_t)2);
  return (Wire.read() << 8) | Wire.read();
}

static void writeRegister(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(adc_addr);
  Wire.write(reg);
  Wire.write(value >> 8);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

static int16_t readChannel(uint8_t ch) {
  uint16_t cfg = (1 << 15)
               | ((0b100 | (ch & 3)) << 12)
               | (0b001 << 9)
               | (1 << 8)
               | (0b100 << 5)
               | 0b0000011;
  writeRegister(CONFIG_REG, cfg);
  delay(10);
  return (int16_t)readRegister(CONV_REG);
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); delay(100);
  digitalWrite(LED, LOW); delay(100);
  digitalWrite(LED, HIGH);

  Serial.begin(115200);
  delay(2000);

  Wire.setPins(17, 20);
  Wire.begin();

  Serial.println(F("=== ADS1x15 Detector ==="));

  // Scan for ADC
  for (byte a = 0x48; a <= 0x4B; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { adc_addr = a; break; }
  }
  if (adc_addr == 0) {
    Serial.println(F("No ADC found"));
    while (1) { digitalWrite(LED, LOW); delay(100); digitalWrite(LED, HIGH); delay(100); }
  }
  Serial.print(F("Address: 0x")); Serial.println(adc_addr, HEX);

  // Identify: read single-ended CH3 (non-zero, stable) and check bottom 4 bits
  uint16_t sum_lsb = 0;
  for (int i = 0; i < 10; i++) {
    uint16_t v = readChannel(3);
    sum_lsb += v & 0x0F;
    delay(20);
  }
  if (sum_lsb == 0)
    Serial.println(F("Device: ADS1015 (12-bit)"));
  else
    Serial.println(F("Device: ADS1115 (16-bit)"));

  Serial.println(F("\nCH0(JoyA-X)\tCH1(JoyA-Y)\tCH2(JoyB-X)\tCH3(JoyB-Y)"));
  Serial.println(F("Raw 16-bit (12-bit left-aligned for ADS1015)"));
}

void loop() {
  digitalWrite(LED, LOW);
  for (int ch = 0; ch < 4; ch++) {
    int16_t v = readChannel(ch);
    Serial.print(v);
    Serial.print(F("\t"));
  }
  Serial.println();
  digitalWrite(LED, HIGH);
  delay(100);
}
