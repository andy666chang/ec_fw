/*
 * @Author: andy.chang
 * @Date: 2025-06-29 17:06:01
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-06-29 19:05:00
 */

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(pwr_n1x, LOG_LEVEL_INFO);

#define MCHP_GPIO_DECLARE(PIN_DT)                                              \
    static const struct gpio_dt_spec PIN_DT =                                  \
        GPIO_DT_SPEC_GET(DT_NODELABEL(PIN_DT), gpios)

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

MCHP_GPIO_DECLARE(ao_3v3_1v8_1v2_en);
MCHP_GPIO_DECLARE(ao_1v8_1v2_pg);
MCHP_GPIO_DECLARE(ao_5v_en);
MCHP_GPIO_DECLARE(ao_pr3v3_en);
MCHP_GPIO_DECLARE(ao_5v_pg);
MCHP_GPIO_DECLARE(ao_pr3v3_pg);
MCHP_GPIO_DECLARE(pmic_en);
MCHP_GPIO_DECLARE(vusb_5v_tpc_pg);
MCHP_GPIO_DECLARE(vio12_usb2_vdd1_en);
MCHP_GPIO_DECLARE(vio12_usb2_vdd1_pg);
MCHP_GPIO_DECLARE(gpu_vr_pg);
MCHP_GPIO_DECLARE(vio18_vdd2l_en);
MCHP_GPIO_DECLARE(vio18_vdd2l_pg);
MCHP_GPIO_DECLARE(pmic_pg);
MCHP_GPIO_DECLARE(pmic_rsmrst);
#ifndef CONFIG_MEC1723_N1X_D_P
MCHP_GPIO_DECLARE(dp12v_en);
MCHP_GPIO_DECLARE(drvvbus_en);
#endif
MCHP_GPIO_DECLARE(vqps_ext_en);
MCHP_GPIO_DECLARE(ovrm_en);

static int power_on(void) {
    int ret;

    LOG_WRN("Run power on sequence: %s", CONFIG_BOARD);

    // ---------------------------------------------
    MCHP_GPIO_SET(&ao_3v3_1v8_1v2_en, 1);
    MCHP_GPIO_WAIT(&ao_1v8_1v2_pg, 1);

    // ---------------------------------------------
    MCHP_GPIO_SET(&ao_5v_en, 1);
    MCHP_GPIO_SET(&ao_pr3v3_en, 1);

    MCHP_GPIO_WAIT(&ao_5v_pg, 1);
    MCHP_GPIO_WAIT(&ao_pr3v3_pg, 1);

    // ---------------------------------------------
    // Check PMIC version before enable PMIC
    // MCHP_GPIO_SET(EC_PMIC_EN_N, 1);
    // LOG_DBG("before PMIC_update, EC_PMIC_EN_N=%d",
    // gpio_read_pin(EC_PMIC_EN_N)); check_PMIC_prog_status(PMIC0_ADDR,
    // Program_PMIC0); check_PMIC_prog_status(PMIC1_ADDR, Program_PMIC1);

    LOG_INF("Enable PMIC");
    MCHP_GPIO_SET(&pmic_en, 0);

    // ---------------------------------------------
    MCHP_GPIO_WAIT(&vusb_5v_tpc_pg, 1);

    // ---------------------------------------------
    MCHP_GPIO_WAIT(&vio12_usb2_vdd1_en, 1);
    MCHP_GPIO_WAIT(&vio12_usb2_vdd1_pg, 1);
    MCHP_GPIO_WAIT(&gpu_vr_pg, 1);
    MCHP_GPIO_WAIT(&vio18_vdd2l_en, 1);
    MCHP_GPIO_WAIT(&vio18_vdd2l_pg, 1);

    // ---------------------------------------------
    MCHP_GPIO_WAIT(&pmic_pg, 1);

    MCHP_GPIO_SET(&pmic_rsmrst, 1);
    // k_work_reschedule(&ec_boot_wd_work, K_SECONDS(1));
    // LOG_INF("%s k_work_reschedule ec_boot_wd_work", __func__);
    MCHP_GPIO_SET(&pmic_en, 1);
    // pwr_err_thread_enable(true);
    // LOG_INF("%s enable power error handler", __func__);
    // ---------------------------------------------
    // EC eSPI initialization
#ifndef CONFIG_MEC1723_N1X_D_P
    MCHP_GPIO_SET(&dp12v_en, 1);
    MCHP_GPIO_SET(&drvvbus_en, 1);
#endif
    MCHP_GPIO_SET(&vqps_ext_en, 1);
    MCHP_GPIO_SET(&ovrm_en, 0);

    // espihub_add_state_handler(pwrseq_slp_handler);
    // espihub_add_warn_handler(ESPIHUB_SUSPEND_WARNING, pwrseq_sus_handler);
    // espihub_add_warn_handler(ESPIHUB_BUS_RESET, espi_bus_reset_handler);

    LOG_WRN("Power sequence Finish!");

    return ret;
}
