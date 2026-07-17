#ifndef _CURIEIMU_H_
#define _CURIEIMU_H_

#include "BMI160.h"

class CurieIMUClass : public BMI160Class {
    public:
        bool begin(void);
        void getMotion6(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz);
        void getAcceleration(int16_t* x, int16_t* y, int16_t* z);
        int16_t getAccelerationX();
        int16_t getAccelerationY();
        int16_t getAccelerationZ();
        int16_t getTemperature();
        void getRotation(int16_t* x, int16_t* y, int16_t* z);
        int16_t getRotationX();
        int16_t getRotationY();
        int16_t getRotationZ();
        uint8_t getDeviceID();
        void attachInterrupt(void (*callback)(void));
        void detachInterrupt(void);

    protected:
        virtual void ss_init();
        virtual int ss_xfer(uint8_t *buf, unsigned tx_cnt, unsigned rx_cnt);

    private:
        int serial_buffer_transfer(uint8_t *buf, unsigned tx_cnt, unsigned rx_cnt);
        void (*_user_callback)(void);
};

#endif
