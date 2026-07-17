#include <Arduino.h>
#include <SPI.h>

#define LED PIN_015

#define CS1 31
#define CS2 29

static void spiWriteReg(uint8_t cs, uint8_t reg, uint8_t val) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs, LOW);
  SPI.transfer(reg & 0x7F);
  SPI.transfer(val);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
}

static uint8_t spiReadReg(uint8_t cs, uint8_t reg) {
  uint8_t val;
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs, LOW);
  SPI.transfer(reg | 0x80);
  val = SPI.transfer(0x00);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
  return val;
}

static bool initBMI160(uint8_t cs) {
  spiWriteReg(cs, 0x7E, 0xB6);
  delay(10);

  spiReadReg(cs, 0x7F);
  delay(10);

  spiWriteReg(cs, 0x7E, 0x11);
  delay(5);
  int timeout = 100;
  while ((spiReadReg(cs, 0x03) & 0x30) != 0x10 && timeout--) {
    delay(1);
  }
  if (timeout <= 0) return false;

  spiWriteReg(cs, 0x7E, 0x15);
  delay(5);
  timeout = 500;
  while ((spiReadReg(cs, 0x03) & 0x0C) != 0x04 && timeout--) {
    delay(1);
  }
  if (timeout <= 0) return false;

  spiWriteReg(cs, 0x41, 0x03);
  spiWriteReg(cs, 0x43, 0x03);

  return spiReadReg(cs, 0x00) == 0xD1;
}

static void readBMI160(uint8_t cs, int16_t* ax, int16_t* ay, int16_t* az,
                       int16_t* gx, int16_t* gy, int16_t* gz) {
  uint8_t buf[12];
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs, LOW);
  SPI.transfer(0x8C);
  for (int i = 0; i < 12; i++) buf[i] = SPI.transfer(0x00);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();

  *gx = (int16_t)((buf[1] << 8) | buf[0]);
  *gy = (int16_t)((buf[3] << 8) | buf[2]);
  *gz = (int16_t)((buf[5] << 8) | buf[4]);
  *ax = (int16_t)((buf[7] << 8) | buf[6]);
  *ay = (int16_t)((buf[9] << 8) | buf[8]);
  *az = (int16_t)((buf[11] << 8) | buf[10]);
}

void setup() {
  Serial.begin(115200);
  Serial.println("BOOT");
  pinMode(LED, OUTPUT);

  SPI.setPins(2, 6, 8);
  SPI.begin();
  pinMode(CS1, OUTPUT); digitalWrite(CS1, HIGH);
  pinMode(CS2, OUTPUT); digitalWrite(CS2, HIGH);

  bool ok1 = initBMI160(CS1);
  bool ok2 = initBMI160(CS2);
  Serial.print("Init S1="); Serial.print(ok1);
  Serial.print(" S2="); Serial.println(ok2);
}

int16_t ax1, ay1, az1, gx1, gy1, gz1;
int16_t ax2, ay2, az2, gx2, gy2, gz2;

void loop() {
  readBMI160(CS1, &ax1, &ay1, &az1, &gx1, &gy1, &gz1);
  readBMI160(CS2, &ax2, &ay2, &az2, &gx2, &gy2, &gz2);

  Serial.print("S1:"); Serial.print(ax1); Serial.print(","); Serial.print(ay1); Serial.print(","); Serial.print(az1);
  Serial.print(","); Serial.print(gx1); Serial.print(","); Serial.print(gy1); Serial.print(","); Serial.print(gz1);
  Serial.print(" | S2:"); Serial.print(ax2); Serial.print(","); Serial.print(ay2); Serial.print(","); Serial.print(az2);
  Serial.print(","); Serial.print(gx2); Serial.print(","); Serial.print(gy2); Serial.print(","); Serial.println(gz2);

  digitalWrite(LED, !digitalRead(LED));
  delay(100);
}
