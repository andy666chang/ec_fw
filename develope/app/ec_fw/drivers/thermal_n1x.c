/*
 * @Author: andy.chang
 * @Date: 2025-06-29 17:06:01
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 02:47:40
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ther_n1x, LOG_LEVEL_INF);

#define FAN_PERIOD PWM_USEC(40) // 25KHz period for fan PWM

static const struct fan_dev_t {
    const struct device *fan_dev;
    const struct device *tach_dev;
} fan_dev_list[] = {
    {
        DEVICE_DT_GET(DT_NODELABEL(pwm0)),
        DEVICE_DT_GET(DT_NODELABEL(tach0)),
    },
    {
        DEVICE_DT_GET(DT_NODELABEL(pwm0)),
        DEVICE_DT_GET(DT_NODELABEL(tach1)),
    },
};

int app_fan_set_speed(int fan_id, uint8_t speed) {
    if (fan_id >= ARRAY_SIZE(fan_dev_list)) {
        LOG_ERR("Invalid fan ID: %d", fan_id);
        return -EINVAL;
    }

    int ret = 0;
    const struct device *fan_dev = fan_dev_list[fan_id].fan_dev;

    ret = pwm_set(fan_dev, 0, FAN_PERIOD, FAN_PERIOD * speed / 100,
                  PWM_POLARITY_INVERTED);
    if (ret < 0) {
        LOG_ERR("Failed to set speed for fan%d: %d", fan_id, ret);
    } else {
        LOG_INF("Set fan%d speed to %d%%", fan_id, speed);
    }

    return ret;
}

int app_fan_get_rpm(int fan_id, uint16_t *rpm) {
    if (fan_id >= ARRAY_SIZE(fan_dev_list)) {
        LOG_ERR("Invalid fan ID: %d", fan_id);
        return -EINVAL;
    }

    int ret = 0;
    const struct device *tach_dev = fan_dev_list[fan_id].tach_dev;
    struct sensor_value val = {0};

    ret = sensor_sample_fetch_chan(tach_dev, SENSOR_CHAN_RPM);
    if (ret) {
        LOG_ERR("Failed to fetch RPM sample for fan%d: %d", fan_id, ret);
        return ret;
    }

    ret = sensor_channel_get(tach_dev, SENSOR_CHAN_RPM, &val);
    if (ret) {
        LOG_ERR("Failed to get RPM for fan%d: %d", fan_id, ret);
        return ret;
    }

    *rpm = (uint16_t)val.val1;
    LOG_INF("Fan%d RPM is %d", fan_id, *rpm);

    return ret;
}

#include <zephyr/init.h>

static int init_config(void) {
    int ret = 0;

    LOG_INF("Initializing thermal configuration...");

    for (size_t i = 0; i < ARRAY_SIZE(fan_dev_list); i++) {
        if (!device_is_ready(fan_dev_list[i].fan_dev)) {
            LOG_ERR("fan%d not ready", i);
            return -ENODEV;
        }

        // Set the PWM period for each fan device
        ret = pwm_set(fan_dev_list[i].fan_dev, 0, FAN_PERIOD, FAN_PERIOD,
                      PWM_POLARITY_INVERTED);
        if (ret < 0) {
            LOG_ERR("Failed to set PWM cycles for fan%d: %d", i, ret);
            return ret;
        }

        if (!device_is_ready(fan_dev_list[i].tach_dev)) {
            LOG_ERR("tach%d not ready", i);
            return -ENODEV;
        }
    }

    return ret;
}

SYS_INIT(init_config, APPLICATION, 0);
