/*
 * @Author: andy.chang 
 * @Date: 2025-07-03 15:07:46 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-03 15:20:22
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "pwr_btn.h"

LOG_MODULE_REGISTER(pwr_btn, LOG_LEVEL_INF);

APP_EVENT_TYPE_DEFINE(pwr_btn_event);

#define BTN_DEBOUNCE_TIME (5)

static const struct gpio_dt_spec pwr_btn0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(pwr_btn_0), gpios);
static struct gpio_callback pwr_btn_cb_data;

/**
 * @brief Power button work
 *
 * @param work
 */
static void pwr_btn_work(struct k_work *work) {
    // Send event
    struct pwr_btn_event evt;

    pwr_btn_event_init(&evt);
    evt.level = gpio_pin_get_dt(&pwr_btn0);

    APP_EVENT_SUBMIT(evt);
}

/**
 * @brief pwr_btn_chg
 *
 */
static K_WORK_DELAYABLE_DEFINE(pwr_btn_chg, pwr_btn_work);

/**
 * @brief Power button interrupt handler (send semaphore)
 *
 * @param dev
 * @param cb
 * @param pins
 */
static void pwr_btn_callback(const struct device *dev, struct gpio_callback *cb,
                             uint32_t pins) {
    // Debounce with kwork
    k_work_reschedule(&pwr_btn_chg, K_MSEC(BTN_DEBOUNCE_TIME));
}

#include <zephyr/init.h>

static int init_config(void) {
    int ret = 0;

    LOG_INF("Initializing pwr_btn configuration...");

    // Initial peripherial
    if (!device_is_ready(pwr_btn0.port)) {
        LOG_ERR("Error: power button  device %s is not ready\n",
                pwr_btn0.port->name);
        return ret;
    }

    ret = gpio_pin_configure_dt(&pwr_btn0, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure %s pin %d\n", ret,
                pwr_btn0.port->name, pwr_btn0.pin);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&pwr_btn0, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret,
                pwr_btn0.port->name, pwr_btn0.pin);
        return ret;
    }

    gpio_init_callback(&pwr_btn_cb_data, pwr_btn_callback, BIT(pwr_btn0.pin));
    gpio_add_callback(pwr_btn0.port, &pwr_btn_cb_data);
    LOG_INF("Set up button at %s pin %d, level: %d\n", pwr_btn0.port->name,
            pwr_btn0.pin, gpio_pin_get_dt(&pwr_btn0));

    return ret;
}

SYS_INIT(init_config, APPLICATION, 0);
