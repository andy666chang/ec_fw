/*
 * @Author: andy.chang 
 * @Date: 2025-04-22 00:48:49 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-03 02:36:48
 */

#pragma once

#include <zephyr/sys/util_macro.h>

typedef struct app_event_type {
    const char *name;
} app_event_type_t;

typedef struct app_event_header {
    const app_event_type_t *type;
} app_event_header_t;

typedef struct app_event_listener {
    const char *name;
    bool (*notification)(const app_event_header_t *aeh);
} app_event_listener_t;

struct app_event_subscriber {
    const app_event_listener_t *listener;
    const app_event_type_t *type;
};

#define APP_EVENT_TYPE_DECLARE(ename)                                          \
    extern const app_event_type_t ename##_type;                                \
    inline void ename##_init(struct ename *evt) {                              \
        evt->header.type = &ename##_type;                                      \
    }                                                                          \
    inline bool is_##ename(const app_event_header_t *aeh) {                    \
        return (aeh->type == &ename##_type);                                   \
    }                                                                          \
    inline struct ename *cast_##ename(const app_event_header_t *aeh) {         \
        return CONTAINER_OF(aeh, struct ename, header);                        \
    }

#define APP_EVENT_TYPE_DEFINE(ename)                                           \
    const app_event_type_t ename##_type = {                                    \
        .name = #ename,                                                        \
    }

#define APP_EVENT_SUBMIT(evt)                                                  \
    do {                                                                       \
        STRUCT_SECTION_FOREACH(app_event_subscriber, p) {                      \
            if (p->type == evt.header.type && p->listener) {                   \
                if (p->listener->notification) {                               \
                    p->listener->notification(&evt.header);                    \
                }                                                              \
            }                                                                  \
        }                                                                      \
    } while (0)

#define APP_EVENT_LISTENER(mname, handler_fn)                                  \
    static const app_event_listener_t _##mname##_app_event_listener = {        \
        .name = #mname,                                                        \
        .notification = handler_fn,                                            \
    }

#define APP_EVENT_SUBSCRIBE(mname, ename)                                      \
    STRUCT_SECTION_ITERABLE(app_event_subscriber,                              \
                            _##ename##_##mname##_app_event_subscriber) = {     \
        .listener = &_##mname##_app_event_listener,                            \
        .type = &ename##_type,                                                 \
    }

///////////////////////////////////////////////////
#if 0
struct sample_event {
    app_event_header_t header;

    int data1;
    int data2;
};

APP_EVENT_TYPE_DECLARE(sample_event);
APP_EVENT_TYPE_DEFINE(sample_event);

// --------------------------------------------------

static bool handler(const app_event_header_t *aeh) {
    if (is_sample_event(aeh)) {
        struct sample_event *evt = cast_sample_event(aeh);
    };
    return 0;
}

APP_EVENT_LISTENER(module123, handler);
APP_EVENT_SUBSCRIBE(module123, sample_event);

APP_EVENT_SUBMIT(aaa);
#endif


