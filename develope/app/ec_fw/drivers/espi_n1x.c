/*
 * @Author: andy.chang
 * @Date: 2025-07-13 03:18:50
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-13 03:47:56
 */

#include <errno.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define ESPI_FREQ_20MHZ 20u
#define ESPI_FREQ_25MHZ 25u
#define ESPI_FREQ_66MHZ 66u

/* eSPI event */
#define EVENT_MASK         0x0000FFFFu
#define EVENT_DETAILS_MASK 0xFFFF0000u
#define EVENT_DETAILS_POS  16u
#define EVENT_TYPE(x)      (x & EVENT_MASK)
#define EVENT_DETAILS(x)   ((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)

LOG_MODULE_REGISTER(espi_n1x, LOG_LEVEL_INF);

static const struct device *const espi_dev = DEVICE_DT_GET(DT_NODELABEL(espi0));
static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
static struct espi_callback kbc_cb;
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
static struct espi_callback oob_cb;
#endif
static uint8_t espi_rst_sts;

static void host_warn_handler(uint32_t signal, uint32_t status) {
    switch (signal) {
    case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
        LOG_INF("Host reset warning %d", status);
        if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
            LOG_INF("HOST RST ACK %d", status);
            espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, status);
        }
        break;
    case ESPI_VWIRE_SIGNAL_SUS_WARN:
        LOG_INF("Host suspend warning %d", status);
        if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
            LOG_INF("SUS ACK %d", status);
            espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_ACK, status);
        }
        break;
    default:
        break;
    }
}

/* eSPI bus event handler */
static void espi_reset_handler(const struct device *dev,
                               struct espi_callback *cb,
                               struct espi_event event) {
    if (event.evt_type == ESPI_BUS_RESET) {
        espi_rst_sts = event.evt_data;
        LOG_INF("eSPI BUS reset %d", event.evt_data);
    }
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(const struct device *dev, struct espi_callback *cb,
                            struct espi_event event) {
    if (event.evt_type == ESPI_BUS_EVENT_CHANNEL_READY) {
        switch (event.evt_details) {
        case ESPI_CHANNEL_VWIRE:
            LOG_INF("VW channel event %x", event.evt_data);
            break;
        case ESPI_CHANNEL_FLASH:
            LOG_INF("Flash channel event %d", event.evt_data);
            break;
        case ESPI_CHANNEL_OOB:
            LOG_INF("OOB channel event %d", event.evt_data);
            break;
        default:
            LOG_ERR("Unknown channel event");
        }
    }
}

/* eSPI vwire received event handler */
static void vwire_handler(const struct device *dev, struct espi_callback *cb,
                          struct espi_event event) {
    if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
        switch (event.evt_details) {
        case ESPI_VWIRE_SIGNAL_PLTRST:
            LOG_INF("PLT_RST changed %d", event.evt_data);
            break;
        case ESPI_VWIRE_SIGNAL_SLP_S3:
        case ESPI_VWIRE_SIGNAL_SLP_S4:
        case ESPI_VWIRE_SIGNAL_SLP_S5:
            LOG_INF("SLP signal changed %d", event.evt_data);
            break;
        case ESPI_VWIRE_SIGNAL_SUS_WARN:
        case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
            host_warn_handler(event.evt_details, event.evt_data);
            break;
        }
    }
}

/* eSPI peripheral channel notifications handler */
static void periph_handler(const struct device *dev, struct espi_callback *cb,
                           struct espi_event event) {
    uint8_t periph_type;
    uint8_t periph_index;

    periph_type = EVENT_TYPE(event.evt_details);
    periph_index = EVENT_DETAILS(event.evt_details);

    switch (periph_type) {
    case ESPI_PERIPHERAL_DEBUG_PORT80:
        LOG_INF("Postcode %x", event.evt_data);
        break;
    case ESPI_PERIPHERAL_HOST_IO:
        LOG_INF("ACPI %x", event.evt_data);
        espi_remove_callback(espi_dev, &p80_cb);
        break;
    default:
        LOG_INF("%s periph 0x%x [%x]", __func__, periph_type, event.evt_data);
    }
}

int app_espi_init(void) {
    int ret;

    /* Indicate to eSPI master simplest configuration: Single line,
     * 20MHz frequency and only logical channel 0 and 1 are supported
     */
    struct espi_cfg cfg = {
        .io_caps = ESPI_IO_MODE_SINGLE_LINE | ESPI_IO_MODE_QUAD_LINES |
                   ESPI_IO_MODE_DUAL_LINES,
        .channel_caps = ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
        .max_freq = ESPI_FREQ_66MHZ,
    };

    if (!device_is_ready(espi_dev)) {
        LOG_ERR("eSPI device %s is not ready", espi_dev->name);
        return -ENODEV;
    }

    LOG_INF("eSPI N1X driver initialized for %s", espi_dev->name);

    // Additional initialization code can be added here
    /* If eSPI driver supports additional capabilities use them */
#ifdef CONFIG_ESPI_OOB_CHANNEL
    cfg.channel_caps |= ESPI_CHANNEL_OOB;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
    cfg.channel_caps |= ESPI_CHANNEL_FLASH;
#endif

    LOG_DBG("About to configure eSPI device %s", __func__);
    ret = espi_config(espi_dev, &cfg);
    if (ret) {
        LOG_ERR("eSPI slave configured failed");
        return ret;
    }

    LOG_INF("eSPI - callbacks initialization... ");
    espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
    espi_init_callback(&vw_rdy_cb, espi_ch_handler,
                       ESPI_BUS_EVENT_CHANNEL_READY);
    espi_init_callback(&vw_cb, vwire_handler, ESPI_BUS_EVENT_VWIRE_RECEIVED);
    espi_init_callback(&p80_cb, periph_handler,
                       ESPI_BUS_PERIPHERAL_NOTIFICATION);
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
    espi_init_callback(&kbc_cb, periph_handler,
                       ESPI_BUS_PERIPHERAL_NOTIFICATION);
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
    espi_init_callback(&oob_cb, oob_rx_handler, ESPI_BUS_EVENT_OOB_RECEIVED);
#endif
    LOG_INF("complete");

    LOG_INF("eSPI - callbacks registration... ");
    espi_add_callback(espi_dev, &espi_bus_cb);
    espi_add_callback(espi_dev, &vw_rdy_cb);
    espi_add_callback(espi_dev, &vw_cb);
    espi_add_callback(espi_dev, &p80_cb);
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
    espi_add_callback(espi_dev, &kbc_cb);
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
    espi_add_callback(espi_dev, &oob_cb);
#endif
    LOG_INF("complete");

    return 0;
}
