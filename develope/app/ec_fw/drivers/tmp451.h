/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 23:55:37 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 23:56:34
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

/* * @brief Initialize the TMP451 sensor.
 *
 * This function initializes the TMP451 sensor by configuring its I2C address
 * and setting up any necessary registers.
 *
 * @param spec Pointer to the I2C device specification.
 * @return 0 on success, negative error code on failure.
 */
int tmp451_init(const struct i2c_dt_spec *spec);

/* * @brief Read the remote temperature from the TMP451 sensor.
 *
 * This function reads the remote temperature from the TMP451 sensor and
 * returns it in the provided pdata pointer.
 *
 * @param spec Pointer to the I2C device specification.
 * @param ch Channel number (not used in this implementation).
 * @param pdata Pointer to store the read temperature data.
 * @return 0 on success, negative error code on failure.
 */
int tmp451_read(const struct i2c_dt_spec *spec, uint8_t ch, uint16_t *pdata);
