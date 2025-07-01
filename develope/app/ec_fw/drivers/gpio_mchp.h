/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 10:42:00 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 10:46:49
 */

#pragma once

#include <zephyr/drivers/gpio.h>

#define MCHP_GPIO_DT_DECLARE(PIN, PIN_DT)                                      \
    static const struct gpio_dt_spec PIN =                                     \
        GPIO_DT_SPEC_GET(DT_NODELABEL(PIN_DT), gpios)

#define MCHP_GPIO_DECLARE(PIN) MCHP_GPIO_DT_DECLARE(PIN, PIN)

#define MCHP_GPIO_SET(PIN, LEVEL)                                              \
    do {                                                                       \
        LOG_INF("Drive %s %s", #PIN, (LEVEL) ? "high" : "low");                \
        int ret = gpio_pin_set_dt(PIN, LEVEL);                                 \
        if (ret) {                                                             \
            LOG_ERR("Failed to write %s", #PIN);                               \
            return ret;                                                        \
        }                                                                      \
    } while (0)

#define MCHP_GPIO_WAIT(PIN, LEVEL)                                             \
    do {                                                                       \
        LOG_INF("Wait %s %s", #PIN, (LEVEL) ? "high" : "low");                 \
        do {                                                                   \
            int ret = gpio_pin_get_dt(PIN);                                    \
            if (ret < 0) {                                                     \
                LOG_ERR("Failed to wait %s", #PIN);                            \
                return ret;                                                    \
            } else if (ret == LEVEL) {                                         \
                break;                                                         \
            }                                                                  \
            k_sleep(K_USEC(100));                                              \
        } while (1);                                                           \
    } while (0)
