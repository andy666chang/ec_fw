/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 02:41:18 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 02:50:35
 */

#pragma once

#include <zephyr/kernel.h>

/** @brief Power on handler
 * 
 * This function is called to handle power on events.
 * It will initiate the power on sequence.
 * 
 * @param obj Pointer to the object that triggered the event.
 * @return int Returns 0 on success, negative error code on failure.
 */
int power_on(void *obj);

/** @brief Power off handler
 * 
 * This function is called to handle power off events.
 * It will initiate the power off sequence.
 * 
 * @return int Returns 0 on success, negative error code on failure.
 */
int power_off(void *obj);

/** @brief Power handler
 * 
 * This function is called to handle power events.
 * It will manage the power state based on the event.
 * 
 * @param obj Pointer to the object that triggered the event.
 * @return int Returns 0 on success, negative error code on failure.
 */
int power_handler(void *obj);
