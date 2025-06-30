/*
 * @Author: andy.chang
 * @Date: 2025-06-29 17:06:01
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-06-29 21:35:50
 */

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(ther_n1x, LOG_LEVEL_INF);

static const struct device *fan_devs[] = {
    DEVICE_DT_GET(DT_NODELABEL(pwm0)),
    DEVICE_DT_GET(DT_NODELABEL(pwm1)),
};

static const struct device *tach_devs[] = {
    DEVICE_DT_GET(DT_NODELABEL(tach0)),
    DEVICE_DT_GET(DT_NODELABEL(tach1)),
};

// static const struct device *tach_dev[TACH_DEV_LIST_SIZE];

#include <zephyr/init.h>

static int init_config(void) {
    int ret = 0;

    LOG_INF("Initializing thermal configuration...");

    for (size_t i = 0; i < ARRAY_SIZE(fan_devs); i++) {
        if (!device_is_ready(fan_devs[i])) {
            LOG_ERR("fan%d not ready", i);
            return -ENODEV;
        }

        // Set the PWM period for each fan device
        ret = pwm_set(fan_devs[i], 0, PWM_USEC(25), PWM_USEC(25),
                      PWM_POLARITY_INVERTED);
        if (ret < 0) {
            LOG_ERR("Failed to set PWM cycles for fan%d: %d", i, ret);
            return ret;
        }
    }

    return ret;
}

SYS_INIT(init_config, APPLICATION, 0);
