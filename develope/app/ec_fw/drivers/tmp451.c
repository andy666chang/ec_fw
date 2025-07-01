/*
 * @Author: andy.chang 
 * @Date: 2025-07-01 22:42:57 
 * @Last Modified by: andy.chang
 * @Last Modified time: 2025-07-01 23:59:30
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tmp451, LOG_LEVEL_INF);

#define TEMP_TMP451_SENSOR1_ID 	 0x00
#define TEMP_TMP451_SENSOR2_ID 	 0x01

#define TEMP_TMP451_SENSOR1_ADDR 0x4C
#define TEMP_TMP451_SENSOR2_ADDR 0x49

#define TMP451_RD_REM_TEMP_LOW_BYTE 0x10
#define TMP451_RD_REM_TEMP_HIGH_BYTE 0x01

#define TMP451_RD_RTHL_TEMPLIMIT_BYTE 0x07	//remote high temp set limit
#define TMP451_RD_LTHL_TEMPLIMIT_BYTE 0x05	//local high temp set limit

#define TMP451_CFG_REG				0x09
#define TMP451_RHIGH_LIMIT_REG		0x0D
#define TMP451_LHIGH_LIMIT_REG		0x0B
#define TMP451_RTHERM_LIMIT_REG		0x19
#define TMP451_LTHERM_LIMIT_REG		0x20
#define TMP451_RD_CFG_REG			0x03

#define THERM2_SEL_MODE_BIT_POS		5u
#define TMP451_SET_PROCHOT_TMP_HIGH_LIMIT		0x67	//103 degrees
#define TMP451_SET_THERMAL_TRIP_TMP_HIGH_LIMIT	0x69	//105 degrees

// Function to convert a raw hexadecimal value to its decimal equivalent
static uint8_t hexToDecimal_conv(uint8_t hexValue) {
    uint8_t decimalValue = 0;
    uint8_t base = 1; // 16^0

    while (hexValue > 0) {
        uint8_t lastDigit = hexValue % 16;
        decimalValue += lastDigit * base;
        base *= 16;
        hexValue /= 16;
    }

    return decimalValue;
}

int tmp451_init(const struct i2c_dt_spec *spec) {
    int ret = 0;

    if (!i2c_is_ready_dt(spec)) {
        LOG_ERR("TMP451 device is not ready");
        return -ENODEV;
    }

    // THERM2 mode
    // Bit [5] = 1 (use pin 6 as THERM2)
    ret = i2c_reg_write_byte_dt(spec, TMP451_CFG_REG,
                                (1 << THERM2_SEL_MODE_BIT_POS));
    if (ret < 0) {
        LOG_ERR("Failed to set THERM2 mode");
        return ret;
    }

    // Therm2 pin state depend on RTHL and LTHL
    // RTHL - Remote temperature high limit
    ret = i2c_reg_write_byte_dt(spec, TMP451_RHIGH_LIMIT_REG,
                                TMP451_SET_PROCHOT_TMP_HIGH_LIMIT);
    if (ret < 0) {
        LOG_ERR("Failed to set remote temperature high limit");
        return ret;
    }

    // LTHL - Local temperature high limit
    ret = i2c_reg_write_byte_dt(spec, TMP451_LHIGH_LIMIT_REG,
                                TMP451_SET_PROCHOT_TMP_HIGH_LIMIT);
    if (ret < 0) {
        LOG_ERR("Failed to set local temperature high limit");
        return ret;
    }

    // Thermal trip event is based on thermal pin (pin4) from TMP451
    // THERMAL TRIP depend on RTH and LTH thermal limits
    // RTHERM - Remote therm trip high limit
    ret = i2c_reg_write_byte_dt(spec, TMP451_RTHERM_LIMIT_REG,
                                TMP451_SET_THERMAL_TRIP_TMP_HIGH_LIMIT);
    if (ret < 0) {
        LOG_ERR("Failed to set remote temperature therm trip high limit");
        return ret;
    }

    // LTHERM - Local therm trip high limit
    ret = i2c_reg_write_byte_dt(spec, TMP451_LTHERM_LIMIT_REG,
                                TMP451_SET_THERMAL_TRIP_TMP_HIGH_LIMIT);
    if (ret < 0) {
        LOG_ERR("Failed to set local temperature therm trip high limit");
        return ret;
    }

    // -------- Read back all settings -------- //
    uint8_t val = 0;
    ret = i2c_reg_read_byte_dt(spec, TMP451_RD_CFG_REG, &val);
    if (ret < 0) {
        LOG_ERR("Failed to read configuration register");
        return ret;
    }
    LOG_INF("configuration register: 0x%x", val);

    ret = i2c_reg_read_byte_dt(spec, TMP451_RD_RTHL_TEMPLIMIT_BYTE, &val);
    if (ret < 0) {
        LOG_ERR("Failed to read configuration register");
        return ret;
    }
    LOG_INF("remote high limit: 0x%x", val);

    ret = i2c_reg_read_byte_dt(spec, TMP451_RD_LTHL_TEMPLIMIT_BYTE, &val);
    if (ret < 0) {
        LOG_ERR("Failed to read configuration register");
        return ret;
    }
    LOG_INF("local high limit: 0x%x", val);

    ret = i2c_reg_read_byte_dt(spec, TMP451_RTHERM_LIMIT_REG, &val);
    if (ret < 0) {
        LOG_ERR("Failed to read configuration register");
        return ret;
    }
    LOG_INF("remote therm limit: 0x%x", val);

    ret = i2c_reg_read_byte_dt(spec, TMP451_LTHERM_LIMIT_REG, &val);
    if (ret < 0) {
        LOG_ERR("Failed to read configuration register");
        return ret;
    }
    LOG_INF("local therm limit: 0x%x", val);

    LOG_INF("TMP451 initialized successfully");
    return 0;
}

// Function to read remote temperature (high byte only) from TMP451 sensor
int tmp451_read(const struct i2c_dt_spec *spec, uint8_t ch, uint16_t *pdata) {
    (void)ch; // ch is not used in this implementation
    uint8_t reg = 0, val = 0, ret = 0;

    if (!pdata) {
        LOG_ERR("TMP451 read failed: NULL pdata");
        return -EINVAL;
    }

    reg = TMP451_RD_REM_TEMP_HIGH_BYTE;

    ret = i2c_reg_read_byte_dt(spec, reg, &val);
    if (ret != 0) {
        LOG_ERR("TMP451 I2C read failed at 0x%02x", spec->addr);
        return ret;
    }

    *pdata = hexToDecimal_conv(val); // only high byte (integer part)
    return ret;
}
