#ifndef _BMI160_H_
#define _BMI160_H_

#include "Arduino.h"

#define BMI160_SPI_READ_BIT         7

#define BMI160_RA_CHIP_ID           0x00

#define BMI160_ACC_PMU_STATUS_BIT   4
#define BMI160_ACC_PMU_STATUS_LEN   2
#define BMI160_GYR_PMU_STATUS_BIT   2
#define BMI160_GYR_PMU_STATUS_LEN   2

#define BMI160_RA_PMU_STATUS        0x03

#define BMI160_RA_GYRO_X_L          0x0C
#define BMI160_RA_GYRO_X_H          0x0D
#define BMI160_RA_GYRO_Y_L          0x0E
#define BMI160_RA_GYRO_Y_H          0x0F
#define BMI160_RA_GYRO_Z_L          0x10
#define BMI160_RA_GYRO_Z_H          0x11
#define BMI160_RA_ACCEL_X_L         0x12
#define BMI160_RA_ACCEL_X_H         0x13
#define BMI160_RA_ACCEL_Y_L         0x14
#define BMI160_RA_ACCEL_Y_H         0x15
#define BMI160_RA_ACCEL_Z_L         0x16
#define BMI160_RA_ACCEL_Z_H         0x17

#define BMI160_STATUS_FOC_RDY       3
#define BMI160_STATUS_NVM_RDY       4
#define BMI160_STATUS_DRDY_GYR      6
#define BMI160_STATUS_DRDY_ACC      7

#define BMI160_RA_STATUS            0x1B

#define BMI160_STEP_INT_BIT         0
#define BMI160_ANYMOTION_INT_BIT    2
#define BMI160_D_TAP_INT_BIT        4
#define BMI160_S_TAP_INT_BIT        5
#define BMI160_NOMOTION_INT_BIT     7
#define BMI160_FFULL_INT_BIT        5
#define BMI160_DRDY_INT_BIT         4
#define BMI160_LOW_G_INT_BIT        3
#define BMI160_HIGH_G_INT_BIT       2

#define BMI160_TAP_SIGN_BIT         7
#define BMI160_TAP_1ST_Z_BIT        6
#define BMI160_TAP_1ST_Y_BIT        5
#define BMI160_TAP_1ST_X_BIT        4

#define BMI160_ANYMOTION_SIGN_BIT   3
#define BMI160_ANYMOTION_1ST_Z_BIT  2
#define BMI160_ANYMOTION_1ST_Y_BIT  1
#define BMI160_ANYMOTION_1ST_X_BIT  0

#define BMI160_HIGH_G_SIGN_BIT      3
#define BMI160_HIGH_G_1ST_Z_BIT     2
#define BMI160_HIGH_G_1ST_Y_BIT     1
#define BMI160_HIGH_G_1ST_X_BIT     0

#define BMI160_RA_INT_STATUS_0      0x1C
#define BMI160_RA_INT_STATUS_1      0x1D
#define BMI160_RA_INT_STATUS_2      0x1E
#define BMI160_RA_INT_STATUS_3      0x1F

#define BMI160_RA_TEMP_L            0x20
#define BMI160_RA_TEMP_H            0x21

#define BMI160_RA_FIFO_LENGTH_0     0x22
#define BMI160_RA_FIFO_LENGTH_1     0x23

#define BMI160_FIFO_DATA_INVALID    0x80
#define BMI160_RA_FIFO_DATA         0x24

#define BMI160_ACCEL_RATE_SEL_BIT    0
#define BMI160_ACCEL_RATE_SEL_LEN    4

#define BMI160_RA_ACCEL_CONF        0X40
#define BMI160_RA_ACCEL_RANGE       0X41

#define BMI160_GYRO_RATE_SEL_BIT    0
#define BMI160_GYRO_RATE_SEL_LEN    4

#define BMI160_RA_GYRO_CONF         0X42
#define BMI160_RA_GYRO_RANGE        0X43

#define BMI160_FIFO_HEADER_EN_BIT   4
#define BMI160_FIFO_ACC_EN_BIT      6
#define BMI160_FIFO_GYR_EN_BIT      7

#define BMI160_RA_FIFO_CONFIG_0     0x46
#define BMI160_RA_FIFO_CONFIG_1     0x47

#define BMI160_ANYMOTION_EN_BIT     0
#define BMI160_ANYMOTION_EN_LEN     3
#define BMI160_D_TAP_EN_BIT         4
#define BMI160_S_TAP_EN_BIT         5
#define BMI160_NOMOTION_EN_BIT      0
#define BMI160_NOMOTION_EN_LEN      3
#define BMI160_LOW_G_EN_BIT         3
#define BMI160_LOW_G_EN_LEN         1
#define BMI160_HIGH_G_EN_BIT        0
#define BMI160_HIGH_G_EN_LEN        3

#define BMI160_STEP_EN_BIT          3
#define BMI160_DRDY_EN_BIT          4
#define BMI160_FFULL_EN_BIT         5

#define BMI160_RA_INT_EN_0          0x50
#define BMI160_RA_INT_EN_1          0x51
#define BMI160_RA_INT_EN_2          0x52

#define BMI160_INT1_EDGE_CTRL       0
#define BMI160_INT1_LVL             1
#define BMI160_INT1_OD              2
#define BMI160_INT1_OUTPUT_EN       3

#define BMI160_RA_INT_OUT_CTRL      0x53

#define BMI160_LATCH_MODE_BIT       0
#define BMI160_LATCH_MODE_LEN       4

#define BMI160_RA_INT_LATCH         0x54
#define BMI160_RA_INT_MAP_0         0x55
#define BMI160_RA_INT_MAP_1         0x56
#define BMI160_RA_INT_MAP_2         0x57

#define BMI160_ANYMOTION_DUR_BIT    0
#define BMI160_ANYMOTION_DUR_LEN    2
#define BMI160_NOMOTION_DUR_BIT     2
#define BMI160_NOMOTION_DUR_LEN     6

#define BMI160_NOMOTION_SEL_BIT     0
#define BMI160_NOMOTION_SEL_LEN     1

#define BMI160_RA_INT_LOWHIGH_0     0x5A
#define BMI160_RA_INT_LOWHIGH_1     0x5B
#define BMI160_RA_INT_LOWHIGH_2     0x5C
#define BMI160_RA_INT_LOWHIGH_3     0x5D
#define BMI160_RA_INT_LOWHIGH_4     0x5E

#define BMI160_RA_INT_MOTION_0      0x5F
#define BMI160_RA_INT_MOTION_1      0x60
#define BMI160_RA_INT_MOTION_2      0x61
#define BMI160_RA_INT_MOTION_3      0x62

#define BMI160_TAP_DUR_BIT          0
#define BMI160_TAP_DUR_LEN          3
#define BMI160_TAP_SHOCK_BIT        6
#define BMI160_TAP_QUIET_BIT        7
#define BMI160_TAP_THRESH_BIT       0
#define BMI160_TAP_THRESH_LEN       5

#define BMI160_RA_INT_TAP_0         0x63
#define BMI160_RA_INT_TAP_1         0x64

#define BMI160_FOC_ACC_Z_BIT        0
#define BMI160_FOC_ACC_Z_LEN        2
#define BMI160_FOC_ACC_Y_BIT        2
#define BMI160_FOC_ACC_Y_LEN        2
#define BMI160_FOC_ACC_X_BIT        4
#define BMI160_FOC_ACC_X_LEN        2
#define BMI160_FOC_GYR_EN           6

#define BMI160_RA_FOC_CONF          0x69

#define BMI160_GYR_OFFSET_X_MSB_BIT 0
#define BMI160_GYR_OFFSET_X_MSB_LEN 2
#define BMI160_GYR_OFFSET_Y_MSB_BIT 2
#define BMI160_GYR_OFFSET_Y_MSB_LEN 2
#define BMI160_GYR_OFFSET_Z_MSB_BIT 4
#define BMI160_GYR_OFFSET_Z_MSB_LEN 2
#define BMI160_ACC_OFFSET_EN        6
#define BMI160_GYR_OFFSET_EN        7

#define BMI160_RA_OFFSET_0          0x71
#define BMI160_RA_OFFSET_1          0x72
#define BMI160_RA_OFFSET_2          0x73
#define BMI160_RA_OFFSET_3          0x74
#define BMI160_RA_OFFSET_4          0x75
#define BMI160_RA_OFFSET_5          0x76
#define BMI160_RA_OFFSET_6          0x77

#define BMI160_RA_STEP_CNT_L        0x78
#define BMI160_RA_STEP_CNT_H        0x79

#define BMI160_STEP_BUF_MIN_BIT     0
#define BMI160_STEP_BUF_MIN_LEN     3
#define BMI160_STEP_CNT_EN_BIT      3

#define BMI160_STEP_TIME_MIN_BIT    0
#define BMI160_STEP_TIME_MIN_LEN    3
#define BMI160_STEP_THRESH_MIN_BIT  3
#define BMI160_STEP_THRESH_MIN_LEN  2
#define BMI160_STEP_ALPHA_BIT       5
#define BMI160_STEP_ALPHA_LEN       3

#define BMI160_RA_STEP_CONF_0       0x7A
#define BMI160_RA_STEP_CONF_1       0x7B

#define BMI160_RA_STEP_CONF_0_NOR   0x15
#define BMI160_RA_STEP_CONF_0_SEN   0x2D
#define BMI160_RA_STEP_CONF_0_ROB   0x1D
#define BMI160_RA_STEP_CONF_1_NOR   0x03
#define BMI160_RA_STEP_CONF_1_SEN   0x00
#define BMI160_RA_STEP_CONF_1_ROB   0x07


#define BMI160_GYRO_RANGE_SEL_BIT   0
#define BMI160_GYRO_RANGE_SEL_LEN   3

#define BMI160_GYRO_RATE_SEL_BIT    0
#define BMI160_GYRO_RATE_SEL_LEN    4

#define BMI160_GYRO_DLPF_SEL_BIT    4
#define BMI160_GYRO_DLPF_SEL_LEN    2

#define BMI160_ACCEL_DLPF_SEL_BIT   4
#define BMI160_ACCEL_DLPF_SEL_LEN   3

#define BMI160_ACCEL_RANGE_SEL_BIT  0
#define BMI160_ACCEL_RANGE_SEL_LEN  4

#define BMI160_CMD_START_FOC        0x03
#define BMI160_CMD_ACC_MODE_NORMAL  0x11
#define BMI160_CMD_GYR_MODE_NORMAL  0x15
#define BMI160_CMD_FIFO_FLUSH       0xB0
#define BMI160_CMD_INT_RESET        0xB1
#define BMI160_CMD_STEP_CNT_CLR     0xB2
#define BMI160_CMD_SOFT_RESET       0xB6

#define BMI160_RA_CMD               0x7E

typedef enum {
    BMI160_LATCH_MODE_NONE = 0,
    BMI160_LATCH_MODE_312_5_US,
    BMI160_LATCH_MODE_625_US,
    BMI160_LATCH_MODE_1_25_MS,
    BMI160_LATCH_MODE_2_5_MS,
    BMI160_LATCH_MODE_5_MS,
    BMI160_LATCH_MODE_10_MS,
    BMI160_LATCH_MODE_20_MS,
    BMI160_LATCH_MODE_40_MS,
    BMI160_LATCH_MODE_80_MS,
    BMI160_LATCH_MODE_160_MS,
    BMI160_LATCH_MODE_320_MS,
    BMI160_LATCH_MODE_640_MS,
    BMI160_LATCH_MODE_1_28_S,
    BMI160_LATCH_MODE_2_56_S,
    BMI160_LATCH_MODE_LATCH,
} BMI160InterruptLatchMode;

typedef enum {
    BMI160_DLPF_MODE_NORM = 0x2,
    BMI160_DLPF_MODE_OSR2 = 0x1,
    BMI160_DLPF_MODE_OSR4 = 0x0,
} BMI160DLPFMode;

typedef enum {
    BMI160_ACCEL_RANGE_2G  = 0X03,
    BMI160_ACCEL_RANGE_4G  = 0X05,
    BMI160_ACCEL_RANGE_8G  = 0X08,
    BMI160_ACCEL_RANGE_16G = 0X0C,
} BMI160AccelRange;

typedef enum {
    BMI160_GYRO_RANGE_2000 = 0,
    BMI160_GYRO_RANGE_1000,
    BMI160_GYRO_RANGE_500,
    BMI160_GYRO_RANGE_250,
    BMI160_GYRO_RANGE_125,
} BMI160GyroRange;

typedef enum {
    BMI160_ACCEL_RATE_25_2HZ = 5,
    BMI160_ACCEL_RATE_25HZ,
    BMI160_ACCEL_RATE_50HZ,
    BMI160_ACCEL_RATE_100HZ,
    BMI160_ACCEL_RATE_200HZ,
    BMI160_ACCEL_RATE_400HZ,
    BMI160_ACCEL_RATE_800HZ,
    BMI160_ACCEL_RATE_1600HZ,
} BMI160AccelRate;

typedef enum {
    BMI160_GYRO_RATE_25HZ = 6,
    BMI160_GYRO_RATE_50HZ,
    BMI160_GYRO_RATE_100HZ,
    BMI160_GYRO_RATE_200HZ,
    BMI160_GYRO_RATE_400HZ,
    BMI160_GYRO_RATE_800HZ,
    BMI160_GYRO_RATE_1600HZ,
    BMI160_GYRO_RATE_3200HZ,
} BMI160GyroRate;

typedef enum {
    BMI160_STEP_MODE_NORMAL = 0,
    BMI160_STEP_MODE_SENSITIVE,
    BMI160_STEP_MODE_ROBUST,
    BMI160_STEP_MODE_UNKNOWN,
} BMI160StepMode;

typedef enum {
    BMI160_TAP_SHOCK_DURATION_50MS = 0,
    BMI160_TAP_SHOCK_DURATION_75MS,
} BMI160TapShockDuration;

typedef enum {
    BMI160_TAP_QUIET_DURATION_30MS = 0,
    BMI160_TAP_QUIET_DURATION_20MS,
} BMI160TapQuietDuration;

typedef enum {
    BMI160_DOUBLE_TAP_DURATION_50MS = 0,
    BMI160_DOUBLE_TAP_DURATION_100MS,
    BMI160_DOUBLE_TAP_DURATION_150MS,
    BMI160_DOUBLE_TAP_DURATION_200MS,
    BMI160_DOUBLE_TAP_DURATION_250MS,
    BMI160_DOUBLE_TAP_DURATION_375MS,
    BMI160_DOUBLE_TAP_DURATION_500MS,
    BMI160_DOUBLE_TAP_DURATION_700MS,
} BMI160DoubleTapDuration;

typedef enum {
    BMI160_ZERO_MOTION_DURATION_1_28S   = 0x00,
    BMI160_ZERO_MOTION_DURATION_2_56S,
    BMI160_ZERO_MOTION_DURATION_3_84S,
    BMI160_ZERO_MOTION_DURATION_5_12S,
    BMI160_ZERO_MOTION_DURATION_6_40S,
    BMI160_ZERO_MOTION_DURATION_7_68S,
    BMI160_ZERO_MOTION_DURATION_8_96S,
    BMI160_ZERO_MOTION_DURATION_10_24S,
    BMI160_ZERO_MOTION_DURATION_11_52S,
    BMI160_ZERO_MOTION_DURATION_12_80S,
    BMI160_ZERO_MOTION_DURATION_14_08S,
    BMI160_ZERO_MOTION_DURATION_15_36S,
    BMI160_ZERO_MOTION_DURATION_16_64S,
    BMI160_ZERO_MOTION_DURATION_17_92S,
    BMI160_ZERO_MOTION_DURATION_19_20S,
    BMI160_ZERO_MOTION_DURATION_20_48S,
    BMI160_ZERO_MOTION_DURATION_25_60S  = 0x10,
    BMI160_ZERO_MOTION_DURATION_30_72S,
    BMI160_ZERO_MOTION_DURATION_35_84S,
    BMI160_ZERO_MOTION_DURATION_40_96S,
    BMI160_ZERO_MOTION_DURATION_46_08S,
    BMI160_ZERO_MOTION_DURATION_51_20S,
    BMI160_ZERO_MOTION_DURATION_56_32S,
    BMI160_ZERO_MOTION_DURATION_61_44S,
    BMI160_ZERO_MOTION_DURATION_66_56S,
    BMI160_ZERO_MOTION_DURATION_71_68S,
    BMI160_ZERO_MOTION_DURATION_76_80S,
    BMI160_ZERO_MOTION_DURATION_81_92S,
    BMI160_ZERO_MOTION_DURATION_87_04S,
    BMI160_ZERO_MOTION_DURATION_92_16S,
    BMI160_ZERO_MOTION_DURATION_97_28S,
    BMI160_ZERO_MOTION_DURATION_102_40S,
    BMI160_ZERO_MOTION_DURATION_112_64S = 0x20,
    BMI160_ZERO_MOTION_DURATION_122_88S,
    BMI160_ZERO_MOTION_DURATION_133_12S,
    BMI160_ZERO_MOTION_DURATION_143_36S,
    BMI160_ZERO_MOTION_DURATION_153_60S,
    BMI160_ZERO_MOTION_DURATION_163_84S,
    BMI160_ZERO_MOTION_DURATION_174_08S,
    BMI160_ZERO_MOTION_DURATION_184_32S,
    BMI160_ZERO_MOTION_DURATION_194_56S,
    BMI160_ZERO_MOTION_DURATION_204_80S,
    BMI160_ZERO_MOTION_DURATION_215_04S,
    BMI160_ZERO_MOTION_DURATION_225_28S,
    BMI160_ZERO_MOTION_DURATION_235_52S,
    BMI160_ZERO_MOTION_DURATION_245_76S,
    BMI160_ZERO_MOTION_DURATION_256_00S,
    BMI160_ZERO_MOTION_DURATION_266_24S,
    BMI160_ZERO_MOTION_DURATION_276_48S,
    BMI160_ZERO_MOTION_DURATION_286_72S,
    BMI160_ZERO_MOTION_DURATION_296_96S,
    BMI160_ZERO_MOTION_DURATION_307_20S,
    BMI160_ZERO_MOTION_DURATION_317_44S,
    BMI160_ZERO_MOTION_DURATION_327_68S,
    BMI160_ZERO_MOTION_DURATION_337_92S,
    BMI160_ZERO_MOTION_DURATION_348_16S,
    BMI160_ZERO_MOTION_DURATION_358_40S,
    BMI160_ZERO_MOTION_DURATION_368_64S,
    BMI160_ZERO_MOTION_DURATION_378_88S,
    BMI160_ZERO_MOTION_DURATION_389_12S,
    BMI160_ZERO_MOTION_DURATION_399_36S,
    BMI160_ZERO_MOTION_DURATION_409_60S,
    BMI160_ZERO_MOTION_DURATION_419_84S,
    BMI160_ZERO_MOTION_DURATION_430_08S,
} BMI160ZeroMotionDuration;

class BMI160Class {
    public:
        void initialize();
        bool testConnection();

        uint8_t getGyroRate();
        void setGyroRate(uint8_t rate);

        uint8_t getAccelRate();
        void setAccelRate(uint8_t rate);

        uint8_t getGyroDLPFMode();
        void setGyroDLPFMode(uint8_t bandwidth);

        uint8_t getAccelDLPFMode();
        void setAccelDLPFMode(uint8_t bandwidth);

        uint8_t getFullScaleGyroRange();
        void setFullScaleGyroRange(uint8_t range);
        uint8_t getFullScaleAccelRange();
        void setFullScaleAccelRange(uint8_t range);

        void autoCalibrateGyroOffset();
        bool getGyroOffsetEnabled();
        void setGyroOffsetEnabled(bool enabled);

        int16_t getXGyroOffset();
        void setXGyroOffset(int16_t offset);
        int16_t getYGyroOffset();
        void setYGyroOffset(int16_t offset);
        int16_t getZGyroOffset();
        void setZGyroOffset(int16_t offset);

        void autoCalibrateXAccelOffset(int target);
        void autoCalibrateYAccelOffset(int target);
        void autoCalibrateZAccelOffset(int target);
        bool getAccelOffsetEnabled();
        void setAccelOffsetEnabled(bool enabled);

        int8_t getXAccelOffset();
        void setXAccelOffset(int8_t offset);
        int8_t getYAccelOffset();
        void setYAccelOffset(int8_t offset);
        int8_t getZAccelOffset();
        void setZAccelOffset(int8_t offset);

        uint8_t getFreefallDetectionThreshold();
        void setFreefallDetectionThreshold(uint8_t threshold);

        uint8_t getFreefallDetectionDuration();
        void setFreefallDetectionDuration(uint8_t duration);

        uint8_t getShockDetectionThreshold();
        void setShockDetectionThreshold(uint8_t threshold);

        uint8_t getShockDetectionDuration();
        void setShockDetectionDuration(uint8_t duration);

        uint8_t getMotionDetectionThreshold();
        void setMotionDetectionThreshold(uint8_t threshold);

        uint8_t getMotionDetectionDuration();
        void setMotionDetectionDuration(uint8_t duration);

        uint8_t getZeroMotionDetectionThreshold();
        void setZeroMotionDetectionThreshold(uint8_t threshold);

        uint8_t getZeroMotionDetectionDuration();
        void setZeroMotionDetectionDuration(uint8_t duration);

        uint8_t getTapDetectionThreshold();
        void setTapDetectionThreshold(uint8_t threshold);

        bool getTapShockDuration();
        void setTapShockDuration(bool duration);

        bool getTapQuietDuration();
        void setTapQuietDuration(bool duration);

        uint8_t getDoubleTapDetectionDuration();
        void setDoubleTapDetectionDuration(uint8_t duration);

        uint8_t getStepDetectionMode();
        void setStepDetectionMode(BMI160StepMode mode);
        bool getStepCountEnabled();
        void setStepCountEnabled(bool enabled);
        uint16_t getStepCount();
        void resetStepCount();

        bool getIntFreefallEnabled();
        void setIntFreefallEnabled(bool enabled);
        bool getIntShockEnabled();
        void setIntShockEnabled(bool enabled);
        bool getIntStepEnabled();
        void setIntStepEnabled(bool enabled);
        bool getIntMotionEnabled();
        void setIntMotionEnabled(bool enabled);
        bool getIntZeroMotionEnabled();
        void setIntZeroMotionEnabled(bool enabled);
        bool getIntTapEnabled();
        void setIntTapEnabled(bool enabled);
        bool getIntDoubleTapEnabled();
        void setIntDoubleTapEnabled(bool enabled);

        bool getGyroFIFOEnabled();
        void setGyroFIFOEnabled(bool enabled);
        bool getAccelFIFOEnabled();
        void setAccelFIFOEnabled(bool enabled);

        bool getIntFIFOBufferFullEnabled();
        void setIntFIFOBufferFullEnabled(bool enabled);
        bool getIntDataReadyEnabled();
        void setIntDataReadyEnabled(bool enabled);

        uint8_t getIntStatus0();
        uint8_t getIntStatus1();
        uint8_t getIntStatus2();
        uint8_t getIntStatus3();
        bool getIntFreefallStatus();
        bool getIntShockStatus();
        bool getIntStepStatus();
        bool getIntMotionStatus();
        bool getIntZeroMotionStatus();
        bool getIntTapStatus();
        bool getIntDoubleTapStatus();
        bool getIntFIFOBufferFullStatus();
        bool getIntDataReadyStatus();

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

        bool getXNegShockDetected();
        bool getXPosShockDetected();
        bool getYNegShockDetected();
        bool getYPosShockDetected();
        bool getZNegShockDetected();
        bool getZPosShockDetected();

        bool getXNegMotionDetected();
        bool getXPosMotionDetected();
        bool getYNegMotionDetected();
        bool getYPosMotionDetected();
        bool getZNegMotionDetected();
        bool getZPosMotionDetected();

        bool getXNegTapDetected();
        bool getXPosTapDetected();
        bool getYNegTapDetected();
        bool getYPosTapDetected();
        bool getZNegTapDetected();
        bool getZPosTapDetected();

        bool getFIFOHeaderModeEnabled();
        void setFIFOHeaderModeEnabled(bool enabled);
        void resetFIFO();

        uint16_t getFIFOCount();
        void getFIFOBytes(uint8_t *data, uint16_t length);

        uint8_t getDeviceID();

        uint8_t getRegister(uint8_t reg);
        void setRegister(uint8_t reg, uint8_t data);

        bool getIntEnabled();
        void setIntEnabled(bool enabled);
        bool getInterruptMode();
        void setInterruptMode(bool mode);
        bool getInterruptDrive();
        void setInterruptDrive(bool drive);
        uint8_t getInterruptLatch();
        void setInterruptLatch(uint8_t latch);
        void resetInterrupt();

    protected:
        virtual int serial_buffer_transfer(uint8_t *buf, unsigned tx_cnt, unsigned rx_cnt);

    private:
        uint8_t reg_read (uint8_t reg);
        void reg_write(uint8_t reg, uint8_t data);
        void reg_write_bits(uint8_t reg, uint8_t data, unsigned pos, unsigned len);
        uint8_t reg_read_bits(uint8_t reg, unsigned pos, unsigned len);
};

#endif
