/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 02:46:45 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-02 00:55:52
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <interfaces/thermal.h>
#include <interfaces/power.h>
#include <drivers/tmp451.h>

LOG_MODULE_REGISTER(thermal, LOG_LEVEL_INF);

#define STACKSIZE 1024
#define PRIORITY 7

static const struct i2c_dt_spec tmp_devs[] = {
    I2C_DT_SPEC_GET(DT_NODELABEL(tmp451_0)),
    I2C_DT_SPEC_GET(DT_NODELABEL(tmp451_1)),
};

enum {
    THERMAL_STATE_IDLE = 0,
    THERMAL_STATE_INITIAL,
    THERMAL_STATE_RUNNING,
    THERMAL_STATE_CLOSE,
};

static K_SEM_DEFINE(thermal_sem, 0, 1);
static uint8_t state = THERMAL_STATE_IDLE;

static void service(void) {

    k_sleep(K_MSEC(100));
    
    while (1) {

        switch (state) {
        case THERMAL_STATE_IDLE: // idle
            LOG_INF("Thermal service is idle, waiting for initialization");
            k_sem_take(&thermal_sem, K_FOREVER);
        case THERMAL_STATE_INITIAL: // initial
            LOG_INF("Thermal service is initializing");
            for (size_t i = 0; i < ARRAY_SIZE(tmp_devs); i++) {
                tmp451_init(&tmp_devs[i]);
            }

            state = THERMAL_STATE_RUNNING; // Change state to running
            LOG_INF("Thermal service is running");
        case THERMAL_STATE_RUNNING: // running
            for (size_t i = 0; i < ARRAY_SIZE(tmp_devs); i++) {
                uint16_t tmp = 0;
                tmp451_read(&tmp_devs[i], 0, &tmp);
                LOG_INF("TMP451[%d] Remote Temperature: %d.%d C", i, tmp / 100,
                        tmp % 100);
            }

            for (size_t i = 0; i < 2; i++) {
                int ret = 0;
                uint16_t rpm = 0;
                uint8_t cnt = 30; // Set fan speed to 30% as an example

                ret = app_fan_set_speed(i, cnt);
                if (ret < 0) {
                    LOG_ERR("Failed to set fan%d speed: %d", i, ret);
                } else {
                    LOG_INF("Fan%d speed set to %d%%", i, cnt);
                }

                ret = app_fan_get_rpm(i, &rpm);
                if (ret < 0) {
                    LOG_ERR("Failed to get fan%d RPM: %d", i, ret);
                } else {
                    LOG_INF("Fan%d RPM is %d", i, rpm);
                }
            }
            k_sem_take(&thermal_sem, K_MSEC(5000));
            break;

        case THERMAL_STATE_CLOSE: // close
            LOG_INF("Thermal service is closing");
            state = 0; // Change state to idle
            break;

        default:
            break;
        }
    }
	
}

K_THREAD_DEFINE(thermal_id, STACKSIZE, service, NULL, NULL, NULL, PRIORITY, 0, 0);


static bool event_handler(const struct app_event_header *aeh) {
    if (is_system_event(aeh)) {
        struct system_event *evt = cast_system_event(aeh);

        if (evt->state == SYSTEM_STATE_S0) {
            // System is waking up, start thermal service
            k_sem_give(&thermal_sem);
        } else if (evt->state == SYSTEM_STATE_S5) {
            // System is going to sleep, stop thermal service
            state = THERMAL_STATE_CLOSE; // Change state to close
            k_sem_give(&thermal_sem);
        }
    }

    return false;
}

APP_EVENT_LISTENER(thermal, event_handler);
APP_EVENT_SUBSCRIBE(thermal, system_event);

#ifdef CONFIG_SHELL
#include <zephyr/shell/shell.h>

static int cmd_fan_set(const struct shell *sh, size_t argc, char **argv) {
    uint8_t ch = 0, duty = 0;

    ch = atoi(argv[1]);
    
    duty = atoi(argv[2]);
    if (duty > 100) {
        duty = 100;
    }

    int ret = app_fan_set_speed(ch, duty);
    if (ret < 0) {
        shell_error(sh, "Failed to set fan%d speed: %d", ch, ret);
    } else {
        shell_info(sh, "Fan%d speed set to %d%%", ch, duty);
    }
    
    return 0;
}

static int cmd_fan_get(const struct shell *sh, size_t argc, char **argv) {
    uint8_t ch = 0;
    uint16_t rpm = 0;

    ch = atoi(argv[1]);

    int ret = app_fan_get_rpm(ch, &rpm);
    if (ret < 0) {
        shell_error(sh, "Failed to get fan%d RPM: %d", ch, ret);
    } else {
        shell_info(sh, "Fan%d RPM is %d", ch, rpm);
    }

    return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_fan,
	SHELL_CMD_ARG(set, NULL,
		"Set fan duty", cmd_fan_set, 2, 0),
	SHELL_CMD_ARG(off, NULL,
		"Get fan RPM", cmd_fan_get, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(fan, &sub_fan, "Power commands", NULL);
#endif
