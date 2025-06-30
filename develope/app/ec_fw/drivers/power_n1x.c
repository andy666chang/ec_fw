/*
 * @Author: andy.chang
 * @Date: 2025-06-29 17:06:01
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 02:51:01
 */

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(pwr_n1x, LOG_LEVEL_INF);

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
MCHP_GPIO_DECLARE(vtr2_thermtrip);

static int _power_off(k_timeout_t delay);

int power_on(void) {
    int ret = 0;

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

int power_off(void) {
    int ret = 0;

    LOG_WRN("%s start", __func__);

    // Pull down THERMTRIP_N -> Wait 300us -> Pull up THERMTRIP_N
    gpio_pin_configure_dt(&vtr2_thermtrip, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN);
    k_sleep(K_USEC(300));
    gpio_pin_configure_dt(&vtr2_thermtrip, GPIO_INPUT | GPIO_INT_EDGE_FALLING);

    // Wait 100ms
    k_sleep(K_MSEC(100));

	// Same as "_power_off"
	_power_off(K_MSEC(0));

    LOG_WRN("%s end", __func__);
    return ret;
}

static int _power_off(k_timeout_t delay) {
    int ret = 0;

    /* 2: wait EC_PMIC_PWR_GD low */
    MCHP_GPIO_WAIT(&pmic_pg, 0);

    /* 4: drive VTR3_SYSRSTB low */
    MCHP_GPIO_SET(&pmic_rsmrst, 0);

    k_sleep(delay);
#ifndef CONFIG_MEC1723_N1X_D_P
    /* 5: write EC_DP12V_EN low */
    MCHP_GPIO_SET(&dp12v_en, 0);

    /* 6: write EC_DRVVBUS_EN low */
    MCHP_GPIO_SET(&drvvbus_en, 0);
#endif

    /* 7: write EC_VQPS_EXT_EN low */
    MCHP_GPIO_SET(&vqps_ext_en, 0);

	/* 8: write EC_OVRM_EN_N high to disable */
	MCHP_GPIO_SET(&ovrm_en, 1);
	
    /* 9: drive EC_AOVCC5V_EN low */
    MCHP_GPIO_SET(&ao_5v_en, 0);

    /* 10: write EC_PR3V3_EN low */
    MCHP_GPIO_SET(&ao_pr3v3_en, 0);

    /* 11: wait EC_AOVCC5V_PG low */
    MCHP_GPIO_WAIT(&ao_5v_pg, 0);

    /* 12: wait EC_VUSB_5V_TPC_PG low */
    MCHP_GPIO_WAIT(&vusb_5v_tpc_pg, 0);

    /* 13: wait EC_PR3V3_PG low */
    MCHP_GPIO_WAIT(&ao_pr3v3_pg, 0);

    /* 14: wait VIO_12_USB2_VDD1_EN low */
    MCHP_GPIO_WAIT(&vio12_usb2_vdd1_en, 0);

    /* 15: wait EC_VIO_12_USB2_VDD1_PG low */
    MCHP_GPIO_WAIT(&vio12_usb2_vdd1_pg, 0);

    /* 16: wait GPU_VR_PGOOD low */
    MCHP_GPIO_WAIT(&gpu_vr_pg, 0);

    /* 17: wait VIO_18_VDD2L_EN low */
    MCHP_GPIO_WAIT(&vio18_vdd2l_en, 0);

    /* 18: wait EC_VIO_18_VDD2L_PG low */
    MCHP_GPIO_WAIT(&vio18_vdd2l_pg, 0);

    /* 19: drive EC_AO_3V3_1V8_1V2_EN low */
    MCHP_GPIO_SET(&ao_3v3_1v8_1v2_en, 0);

    /* 20: wait EC_AO1V8_AO1V2_PG low */
    MCHP_GPIO_WAIT(&ao_1v8_1v2_pg, 0);

    return ret;
}

#include <zephyr/init.h>

MCHP_GPIO_DECLARE(flash_mux_path_ctl);

const struct pins_cfg_t {
    const struct gpio_dt_spec *pin;
    uint32_t flags;
} pins_cfg[] = {
    {&ao_3v3_1v8_1v2_en, GPIO_OUTPUT_LOW},
    {&ao_1v8_1v2_pg, GPIO_INPUT},
    {&ao_5v_en, GPIO_OUTPUT_LOW},
    {&ao_pr3v3_en, GPIO_OUTPUT_LOW},
    {&ao_5v_pg, GPIO_INPUT},
    {&ao_pr3v3_pg, GPIO_INPUT},
    {&pmic_en, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN},
    {&vusb_5v_tpc_pg, GPIO_INPUT},
    {&vio12_usb2_vdd1_en, GPIO_INPUT},
    {&vio12_usb2_vdd1_pg, GPIO_INPUT},
    {&gpu_vr_pg, GPIO_INPUT},
    {&vio18_vdd2l_en, GPIO_INPUT},
    {&vio18_vdd2l_pg, GPIO_INPUT},
    {&pmic_pg, GPIO_INPUT},
    {&pmic_rsmrst, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN},
#ifndef CONFIG_MEC1723_N1X_D_P
    {&dp12v_en, GPIO_OUTPUT_LOW},
    {&drvvbus_en, GPIO_OUTPUT_LOW},
#endif
    {&vqps_ext_en, GPIO_OUTPUT_LOW},
    {&ovrm_en, GPIO_OUTPUT_HIGH},
    {&vtr2_thermtrip, GPIO_INPUT},
    {&flash_mux_path_ctl, GPIO_OUTPUT_LOW},
};

static int pwr_on_config(void) {
    int ret = 0;

    LOG_INF("Configuring power on pins...");

    for (size_t i = 0; i < ARRAY_SIZE(pins_cfg); i++) {
        ret = gpio_pin_configure_dt(pins_cfg[i].pin, pins_cfg[i].flags);
        if (ret < 0) {
            LOG_ERR("Failed to configure %s: %d", pins_cfg[i].pin->port->name,
                    ret);
            break;
        }
    }

    return ret;
}

SYS_INIT(pwr_on_config, APPLICATION, 0);
