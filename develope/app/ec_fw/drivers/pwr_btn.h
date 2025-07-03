/*
 * @Author: andy.chang 
 * @Date: 2025-07-03 15:07:46 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-03 15:10:14
 */

#pragma once

#include <app_event_manager.h>

struct pwr_btn_event {
    app_event_header_t header;

    bool level;
};

APP_EVENT_TYPE_DECLARE(pwr_btn_event);
