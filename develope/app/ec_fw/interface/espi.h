/*
 * @Author: andy.chang 
 * @Date: 2025-07-13 02:50:26 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-19 16:51:50
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>

#include <app_event_manager.h>


struct espi_rst_event {
    app_event_header_t header;

    uint32_t data;
};

APP_EVENT_TYPE_DECLARE(espi_rst_event);

struct espi_ch_event {
    app_event_header_t header;

    enum espi_channel ch;
    uint32_t data;
};

APP_EVENT_TYPE_DECLARE(espi_ch_event);

struct espi_vwire_event {
    app_event_header_t header;

    enum espi_vwire_signal vwire;
    uint32_t data;
};

APP_EVENT_TYPE_DECLARE(espi_vwire_event);

struct espi_periph_event {
    app_event_header_t header;

    enum espi_virtual_peripheral type;
    uint8_t index;
    uint32_t data;
};

APP_EVENT_TYPE_DECLARE(espi_periph_event);
