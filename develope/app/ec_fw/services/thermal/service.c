/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 02:46:45 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 11:02:40
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <interfaces/thermal.h>

LOG_MODULE_REGISTER(thermal, LOG_LEVEL_INF);

#define STACKSIZE 1024
#define PRIORITY 7

static void service(void) {
    LOG_INF("Thermal service started");

    // Initialize thermal sensors, fans, etc.
    // int ret = thermal_init();
    // if (ret < 0) {
    //     LOG_ERR("Failed to initialize thermal service: %d", ret);
    //     return;
    // }

    while (1) {
        // Monitor temperature and control fans
        // ret = thermal_monitor();
        // if (ret < 0) {
        //     LOG_ERR("Thermal monitoring failed: %d", ret);
        // }

        k_sleep(K_MSEC(1000)); // Sleep for 1 second before next iteration
    }
	
}

K_THREAD_DEFINE(thermal_id, STACKSIZE, service, NULL, NULL, NULL, PRIORITY, 0, 0);

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
