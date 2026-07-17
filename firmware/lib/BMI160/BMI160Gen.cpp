#include "BMI160Gen.h"
#include "SPI.h"
#include "Wire.h"

bool BMI160GenClass::begin(const int spi_cs_pin, const int intr_pin)
{
    return begin(SPI_MODE, spi_cs_pin, intr_pin);
}

bool BMI160GenClass::begin(Mode mode, const int arg1, const int arg2)
{
    this->mode = mode;
    switch (this->mode) {
    case INVALID_MODE:
        return false;
    case I2C_MODE:
        i2c_addr = arg1;
        break;
    case SPI_MODE:
        spi_ss = arg1;
        break;
    default:
        return false;
    }
    if (0 <= arg2) {
        interrupt_pin = digitalPinToInterrupt(arg2);
    }
    return CurieIMUClass::begin();
}

void BMI160GenClass::attachInterrupt(void (*callback)(void))
{
    CurieIMUClass::attachInterrupt(NULL);
    if (0 <= interrupt_pin) {
        ::attachInterrupt(interrupt_pin, callback, FALLING);
    }
}

void BMI160GenClass::ss_init()
{
    switch (this->mode) {
    case I2C_MODE:
        i2c_init();
        break;
    case SPI_MODE:
        spi_init();
        break;
    default:
        break;
    }
}

int BMI160GenClass::ss_xfer(uint8_t *buf, unsigned tx_cnt, unsigned rx_cnt)
{
    switch (this->mode) {
    case I2C_MODE:
        return i2c_xfer(buf, tx_cnt, rx_cnt);
    case SPI_MODE:
        if (rx_cnt)
            buf[0] |= (1 << BMI160_SPI_READ_BIT);
        return spi_xfer(buf, tx_cnt, rx_cnt);
    default:
        return 0;
    }
}

void BMI160GenClass::i2c_init()
{
    Wire.begin();
    Wire.beginTransmission(i2c_addr);
    Wire.endTransmission();
}

int BMI160GenClass::i2c_xfer(uint8_t *buf, unsigned tx_cnt, unsigned rx_cnt)
{
    uint8_t *p;
    Wire.beginTransmission(i2c_addr);
    p = buf;
    while (0 < tx_cnt) {
        tx_cnt--;
        Wire.write(*p++);
    }
    Wire.endTransmission();
    if (0 < rx_cnt) {
        Wire.requestFrom(i2c_addr, rx_cnt);
        p = buf;
        while (Wire.available() && 0 < rx_cnt) {
            rx_cnt--;
            *p++ = Wire.read();
        }
    }
    return 0;
}

void BMI160GenClass::spi_init()
{
    SPI.begin();
    if (0 <= spi_ss) {
        pinMode(spi_ss, OUTPUT);
        digitalWrite(spi_ss, HIGH);
    }
}

int BMI160GenClass::spi_xfer(uint8_t *buf, unsigned tx_cnt, unsigned rx_cnt)
{
    uint8_t *p;
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    if (0 <= spi_ss)
        digitalWrite(spi_ss, LOW);
    p = buf;
    while (0 < tx_cnt) {
        tx_cnt--;
        SPI.transfer(*p++);
    }
    p = buf;
    while (0 < rx_cnt) {
        rx_cnt--;
        *p++ = SPI.transfer(0);
    }
    if (0 <= spi_ss)
        digitalWrite(spi_ss, HIGH);
    SPI.endTransaction();
    return 0;
}
