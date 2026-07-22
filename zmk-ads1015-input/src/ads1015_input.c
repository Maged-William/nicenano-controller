#define DT_DRV_COMPAT zmk_ads1015_input

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ads1015_input, CONFIG_ZMK_ADS1015_INPUT_LOG_LEVEL);

#define ADS1015_CONV_REG   0x00
#define ADS1015_CONFIG_REG 0x01

#define ADS1015_OS_SINGLE    BIT(15)
#define ADS1015_MUX(ch)      ((0b100 | ((ch) & 3)) << 12)
#define ADS1015_PGA_4_096V   (0b001 << 9)
#define ADS1015_MODE_SINGLE  BIT(8)
#define ADS1015_DR_2400SPS   (5 << 5)
#define ADS1015_COMP_DISABLE 0x03

struct ads1015_input_config {
    const struct device *i2c_bus;
    uint16_t i2c_addr;
    uint8_t ch_x;
    uint8_t ch_y;
    uint32_t interval_ms;
};

struct ads1015_input_data {
    struct k_work_delayable work;
    const struct device *dev;
    int16_t center_x;
    int16_t center_y;
    bool calibrated;
    int32_t smooth_x;
    int32_t smooth_y;
};

static int ads1015_write_reg(const struct device *i2c, uint16_t addr,
                              uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = { reg, val >> 8, val & 0xFF };
    return i2c_write(i2c, buf, 3, addr);
}

static int ads1015_read_reg(const struct device *i2c, uint16_t addr,
                             uint8_t reg, uint16_t *val)
{
    uint8_t rx[2];
    int ret = i2c_write_read(i2c, addr, &reg, 1, rx, 2);
    if (ret == 0) {
        *val = ((uint16_t)rx[0] << 8) | rx[1];
    }
    return ret;
}

static int16_t ads1015_read_channel(const struct device *i2c, uint16_t addr,
                                     uint8_t ch)
{
    uint16_t cfg = ADS1015_OS_SINGLE | ADS1015_MUX(ch)
                 | ADS1015_PGA_4_096V | ADS1015_MODE_SINGLE
                 | ADS1015_DR_2400SPS | ADS1015_COMP_DISABLE;

    if (ads1015_write_reg(i2c, addr, ADS1015_CONFIG_REG, cfg) != 0) {
        return 0;
    }

    uint16_t status;
    int timeout = 100;
    do {
        if (ads1015_read_reg(i2c, addr, ADS1015_CONFIG_REG, &status) != 0) {
            return 0;
        }
        if (--timeout <= 0) {
            LOG_WRN("ADS1015 channel %d timeout", ch);
            return 0;
        }
    } while (!(status & ADS1015_OS_SINGLE));

    uint16_t raw;
    if (ads1015_read_reg(i2c, addr, ADS1015_CONV_REG, &raw) != 0) {
        return 0;
    }
    return (int16_t)raw;
}

static void ads1015_poll_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct ads1015_input_data *data =
        CONTAINER_OF(dwork, struct ads1015_input_data, work);
    const struct ads1015_input_config *cfg = data->dev->config;

    int16_t x = ads1015_read_channel(cfg->i2c_bus, cfg->i2c_addr, cfg->ch_x);
    int16_t y = ads1015_read_channel(cfg->i2c_bus, cfg->i2c_addr, cfg->ch_y);

    if (!data->calibrated) {
        data->center_x = x;
        data->center_y = y;
        data->smooth_x = x;
        data->smooth_y = y;
        data->calibrated = true;
        LOG_INF("ADS1015 center: X=%d Y=%d", data->center_x, data->center_y);
    } else {
        data->smooth_x = (data->smooth_x * 7 + x) / 8;
        data->smooth_y = (data->smooth_y * 7 + y) / 8;
    }

    int16_t dx_raw = (int16_t)(data->smooth_x - data->center_x);
    int16_t dy_raw = (int16_t)(data->smooth_y - data->center_y);

    int8_t target_x = CLAMP(dx_raw / 16, -127, 127);
    int8_t target_y = CLAMP(dy_raw / 16, -127, 127);

    if (target_x > -8 && target_x < 8) target_x = 0;
    if (target_y > -8 && target_y < 8) target_y = 0;

    input_report(data->dev, INPUT_EV_ABS, INPUT_ABS_X, target_x, false, K_NO_WAIT);
    input_report(data->dev, INPUT_EV_ABS, INPUT_ABS_Y, target_y, true, K_NO_WAIT);

    LOG_DBG("joy abs X=%d Y=%d", target_x, target_y);

    k_work_schedule(&data->work, K_MSEC(cfg->interval_ms));
}

static int ads1015_input_init(const struct device *dev)
{
    const struct ads1015_input_config *cfg = dev->config;
    struct ads1015_input_data *data = dev->data;

    data->dev = dev;

    if (!device_is_ready(cfg->i2c_bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    if (i2c_write(cfg->i2c_bus, NULL, 0, cfg->i2c_addr) != 0) {
        LOG_ERR("ADS1015 not found at 0x%02x", cfg->i2c_addr);
        return -ENODEV;
    }

    LOG_INF("ADS1015 found at 0x%02x, X:ch%d Y:ch%d interval:%dms",
            cfg->i2c_addr, cfg->ch_x, cfg->ch_y, cfg->interval_ms);

    k_work_init_delayable(&data->work, ads1015_poll_handler);
    k_work_schedule(&data->work, K_MSEC(100));

    return 0;
}

#define ADS1015_INIT(n) \
    static const struct ads1015_input_config ads1015_input_config_##n = { \
        .i2c_bus = DEVICE_DT_GET(DT_INST_BUS(n)), \
        .i2c_addr = DT_INST_REG_ADDR(n), \
        .ch_x = DT_INST_PROP_OR(n, channel_x, 0), \
        .ch_y = DT_INST_PROP_OR(n, channel_y, 1), \
        .interval_ms = DT_INST_PROP_OR(n, update_interval_ms, 3), \
    }; \
    static struct ads1015_input_data ads1015_input_data_##n; \
    DEVICE_DT_INST_DEFINE(n, ads1015_input_init, NULL, \
                          &ads1015_input_data_##n, \
                          &ads1015_input_config_##n, \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(ADS1015_INIT)
