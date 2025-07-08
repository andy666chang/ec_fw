/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 02:46:45 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-03 15:23:15
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <interfaces/power.h>
#include <drivers/pwr_btn.h>

LOG_MODULE_REGISTER(power, LOG_LEVEL_INF);

#define STACKSIZE 1024
#define PRIORITY 7
#define POWER_OFF_DELAY (10)

K_SEM_DEFINE(power_sem, 0, 1);

static bool power_off_pending = false;
static int countdown = POWER_OFF_DELAY;

static bool state = false;

// Power service thread
static void service(void) {
    while (1) {
        k_sem_take(&power_sem, K_FOREVER);

        if (power_off_pending) {
            LOG_INF(">> Executing POWER OFF!");
            power_off();
            state = false;
        } else {
            LOG_INF(">> Executing POWER ON!");
            power_on();
            state = true;
        }
    }
}

K_THREAD_DEFINE(power_id, STACKSIZE, service, NULL, NULL, NULL, PRIORITY, 0, 0);


// count down timer handler：1s
static void countdown_timer_handler(struct k_timer *timer) {
    if (--countdown > 0) {
        LOG_INF("Countdown: %d", countdown);
    } else {
        power_off_pending = true;
        k_sem_give(&power_sem);
        k_timer_stop(timer);
    }
}
K_TIMER_DEFINE(countdown_timer, countdown_timer_handler, NULL);

static bool event_handler(const struct app_event_header *aeh) {
    if (is_pwr_btn_event(aeh)) {
        struct pwr_btn_event *evt = cast_pwr_btn_event(aeh);

        if (evt->level == 0) { // pressed
            LOG_INF("Power button: pressed");

            if (state == false) { // S5
                // Do power on
                power_off_pending = false;
                k_sem_give(&power_sem);
            } else { // S0
                // Start timer
                countdown = POWER_OFF_DELAY;
                k_timer_start(&countdown_timer, K_SECONDS(1), K_SECONDS(1));
            }
        } else { // released
            LOG_INF("Power button: released");

            // Cancel timer
            k_timer_stop(&countdown_timer);
        }
    }

    return false;
}

APP_EVENT_LISTENER(power, event_handler);
APP_EVENT_SUBSCRIBE(power, pwr_btn_event);

#ifdef CONFIG_SHELL
#include <zephyr/shell/shell.h>

static int cmd_pwr_on(const struct shell *sh, size_t argc, char **argv) {
    int ret = power_on();
    return ret;
}

static int cmd_pwr_off(const struct shell *sh, size_t argc, char **argv) {
    int ret = power_off();
    return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_power,
	SHELL_CMD_ARG(on, NULL,
		"Power on system", cmd_pwr_on, 0, 0),
	SHELL_CMD_ARG(off, NULL,
		"Power off system", cmd_pwr_off, 0, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(power, &sub_power, "Power commands", NULL);
#endif
