#include "CurieIMU.h"

#define CURIE_IMU_CHIP_ID 0xD1

bool CurieIMUClass::begin()
{
    ss_init();

    uint8_t dummy_reg = 0x7F;
    serial_buffer_transfer(&dummy_reg, 1, 1);

    BMI160Class::initialize();

    return (CURIE_IMU_CHIP_ID == getDeviceID());
}

void CurieIMUClass::getMotion6(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz)
{
    BMI160Class::getMotion6(ax, ay, az, gx, gy, gz);
}

void CurieIMUClass::getAcceleration(int16_t* x, int16_t* y, int16_t* z)
{
    BMI160Class::getAcceleration(x, y, z);
}

int16_t CurieIMUClass::getAccelerationX()
{
    return BMI160Class::getAccelerationX();
}

int16_t CurieIMUClass::getAccelerationY()
{
    return BMI160Class::getAccelerationY();
}

int16_t CurieIMUClass::getAccelerationZ()
{
    return BMI160Class::getAccelerationZ();
}

int16_t CurieIMUClass::getTemperature()
{
    return BMI160Class::getTemperature();
}

void CurieIMUClass::getRotation(int16_t* x, int16_t* y, int16_t* z)
{
    BMI160Class::getRotation(x, y, z);
}

int16_t CurieIMUClass::getRotationX()
{
    return BMI160Class::getRotationX();
}

int16_t CurieIMUClass::getRotationY()
{
    return BMI160Class::getRotationY();
}

int16_t CurieIMUClass::getRotationZ()
{
    return BMI160Class::getRotationZ();
}

uint8_t CurieIMUClass::getDeviceID()
{
    return BMI160Class::getDeviceID();
}

int CurieIMUClass::serial_buffer_transfer(uint8_t *buf, unsigned tx_cnt, unsigned rx_cnt)
{
    return ss_xfer(buf, tx_cnt, rx_cnt);
}

void CurieIMUClass::attachInterrupt(void (*callback)(void))
{
    _user_callback = callback;
    setInterruptMode(1);
    setInterruptDrive(0);
    setInterruptLatch(BMI160_LATCH_MODE_10_MS);
    setIntEnabled(true);
}

void CurieIMUClass::detachInterrupt(void)
{
    setIntEnabled(false);
}
