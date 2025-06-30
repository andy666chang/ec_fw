/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 02:46:45 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 02:49:48
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <interfaces/power.h>

LOG_MODULE_REGISTER(power, LOG_LEVEL_INF);



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
