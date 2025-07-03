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



static bool handler(const app_event_header_t *aeh) {
    if (is_pwr_btn_event(aeh)) {
        struct pwr_btn_event *evt = cast_pwr_btn_event(aeh);
		LOG_INF("power button level: %d", evt->level);
    };
    return 0;
}

APP_EVENT_LISTENER(power, handler);
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
