/*
 * @Author: andy.chang 
 * @Date: 2025-07-13 02:50:26 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-13 02:50:51
 */

#pragma once

#include <zephyr/kernel.h>

#include <app_event_manager.h>

enum system_state{
    SYSTEM_STATE_S0,
    SYSTEM_STATE_S5,
    SYSTEM_STATE_MAX,
};

struct system_event {
    app_event_header_t header;

    enum system_state state;
};

APP_EVENT_TYPE_DECLARE(system_event);
