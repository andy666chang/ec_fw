/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 10:44:47 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 10:54:14
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "gpio_mchp.h"

LOG_MODULE_REGISTER(board_n1x, LOG_LEVEL_INF);

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
MCHP_GPIO_DECLARE(flash_mux_path_ctl);
MCHP_GPIO_DECLARE(wake_sci);
MCHP_GPIO_DECLARE(kbc_caps_lock);
MCHP_GPIO_DECLARE(cam_lid_close);
MCHP_GPIO_DECLARE(cam_plt_rst);
MCHP_GPIO_DECLARE(s4_led);
MCHP_GPIO_DECLARE(ms_led);
MCHP_GPIO_DECLARE(cam0_active);
MCHP_GPIO_DECLARE(s0_led);
MCHP_GPIO_DECLARE(rtc_en);

typedef struct gpio_cfg_t {
    const struct gpio_dt_spec *pin;
    uint32_t flags;
} gpio_cfg_t;

static const gpio_cfg_t gpio_init_cfg_tbl[] = {
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
    {&wake_sci, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN},     // EC_GPIO_052
    {&kbc_caps_lock, GPIO_OUTPUT_LOW},                   // EC_GPIO_062
    {&cam_lid_close, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN}, // EC_GPIO_067
    {&cam_plt_rst, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN},   // EC_GPIO_064
    {&s4_led, GPIO_OUTPUT_LOW},                          // EC_GPIO_141
    {&ms_led, GPIO_OUTPUT_LOW},                          // EC_GPIO_145
    {&cam0_active, GPIO_OUTPUT_LOW},                     // EC_GPIO_153
    {&s0_led, GPIO_OUTPUT_LOW},                          // EC_GPIO_165
    {&rtc_en, GPIO_OUTPUT_HIGH},                         // EC_GPIO_101
};

static int init(void) {
    int ret = 0;

    LOG_INF("Configuring power on pins...");

    for (size_t i = 0; i < ARRAY_SIZE(gpio_init_cfg_tbl); i++) {
        const gpio_cfg_t *cfg = &gpio_init_cfg_tbl[i];

        if (!device_is_ready(cfg->pin->port)) {
            LOG_ERR("GPIO port %s is not ready", cfg->pin->port->name);
            ret = -ENODEV;
            break;
        }

        ret = gpio_pin_configure_dt(cfg->pin, cfg->flags);
        if (ret < 0) {
            LOG_ERR("Failed to configure %s: %d", cfg->pin->port->name, ret);
            break;
        }
    }

    return ret;
}

SYS_INIT(init, APPLICATION, 0);


static uintptr_t vci_regbase = DT_REG_ADDR(DT_NODELABEL(vci0));
static int vci_config(void) {
    struct vci_regs *VCI_REGS = (struct vci_regs *)vci_regbase;

	LOG_INF("VCI_REGS->CONFIG: 0x%08x", VCI_REGS->CONFIG);

    // Enable FW control & Set VOUT2 to HIGH
    VCI_REGS->CONFIG |= MCHP_VCI_FW_CTRL_EN;
	VCI_REGS->CONFIG |= MCHP_VCI_FW_EXT_SEL;

	LOG_INF("VCI_REGS->CONFIG: 0x%08x", VCI_REGS->CONFIG);

    return 0;
}

SYS_INIT(vci_config, PRE_KERNEL_1, 0);

/* Set bits 0,1 and 2 to enable BGPO0, BGPO1 and BGPO2 */
#define BGPO_EN_MASK 0x7U
/* Set bits 0-4 to enable BGPO0-BGPO5 */
#define BGPO_EN_MASK 0x3FU
static uintptr_t wktmr_regbase = DT_REG_ADDR(DT_NODELABEL(weektmr0));
static int bgpo_disable(void) {
    struct wktmr_regs *regs = (struct wktmr_regs *)wktmr_regbase;

    uint32_t data = regs->BGPO_PWR;
    /* Clear mask to disable BGPO */
    data &= ~(BGPO_EN_MASK);
    regs->BGPO_PWR = data;
}

SYS_INIT(bgpo_disable, PRE_KERNEL_1, 0);
