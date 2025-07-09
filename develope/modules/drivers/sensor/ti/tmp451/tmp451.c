/*
 * Copyright (c) 2024 Bittium Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp451

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "tmp451.h"

LOG_MODULE_REGISTER(TMP451, CONFIG_SENSOR_LOG_LEVEL);

static inline int tmp451_reg_read(const struct tmp451_config *cfg, uint8_t reg, uint8_t *buf,
				  uint32_t size)
{
	return i2c_burst_read_dt(&cfg->i2c, reg, buf, size);
}

static inline int tmp451_reg_write(const struct tmp451_config *cfg, uint8_t reg, uint8_t *buf,
				   uint32_t size)
{
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, size);
}

static inline int tmp451_get_status(const struct tmp451_config *cfg, uint8_t *status)
{
	return tmp451_reg_read(cfg, TMP451_STATUS_REG, status, 1);
}

static int tmp451_one_shot(const struct device *dev)
{
    uint8_t data = 0;
    uint8_t status = 0;
    int ret = 0;
    const struct tmp451_config *cfg = dev->config;

    data = 1; /* write anything to start [49] */
    ret = tmp451_reg_write(cfg, TMP451_ONE_SHOT_START_REG, &data, 1);
    if (ret < 0) {
        LOG_ERR("Failed to write TMP451_ONE_SHOT_START_REG ret:%d", ret);
        return ret;
    }

    for (uint16_t i = 0; i < TMP451_CONV_LOOP_LIMIT; i++) {
        ret = tmp451_get_status(cfg, &status);
        if (ret < 0) {
            LOG_DBG("Failed to read TMP451_STATUS_REG, ret:%d", ret);
        } else {
            if (status & TMP451_STATUS_REG_BUSY) {
                /* conversion not ready */
                k_msleep(10);
            } else {
                LOG_DBG("conv over, loops:%d status:%x", i, status);
                break;
            }
        }
    }
    return ret; // 返回最後一次讀取狀態的結果，如果沒有轉換完成，則可能返回錯誤
}

static int tmp451_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    int ret = 0;
    uint8_t value_h = 0; // 高位元組
    uint8_t value_l = 0; // 低位元組 (高4位元有效)
    int32_t temp_raw = 0; // 原始溫度數據 (12位元)

    const struct tmp451_config *cfg = dev->config;
    struct tmp451_data *data = dev->data;

    // 檢查通道支援
    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP &&
        chan != SENSOR_CHAN_AMBIENT_TEMP) {
        return -ENOTSUP;
    }

    // 如果設備處於關機模式，先觸發一次性轉換
    // (在 tmp451_init 中預設設定為連續轉換模式，如果應用需要單次模式，則此處需要判斷)
    // tmp451_one_shot(dev); // 如果配置為 One-shot mode，則需調用此函數

    // --- 讀取本地溫度 ---
    if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP)) {
        // 先讀取高位元組，再讀取低位元組，以保證數據一致性
        ret = tmp451_reg_read(cfg, TMP451_LOCAL_TEMP_H_REG, &value_h, sizeof(value_h));
        if (ret < 0) {
            LOG_ERR("Failed to read TMP451_LOCAL_TEMP_H_REG, ret:%d", ret);
            return ret;
        }

        ret = tmp451_reg_read(cfg, TMP451_LOCAL_TEMP_L_REG, &value_l, sizeof(value_l));
        if (ret < 0) {
            LOG_ERR("Failed to read TMP451_LOCAL_TEMP_L_REG, ret:%d", ret);
            return ret;
        }

        // 組合 12 位元原始數據 [23]
        // TMP451_LOCAL_TEMP_L_REG (0x15) 的高 4 位元 (LT3-LT0) 是小數部分
        temp_raw = ((int32_t)value_h << 4) | ((value_l >> 4) & 0x0F);
        
        // 處理負溫度和擴展範圍，並將原始 12 位元數據轉換為 Zephyr 內部表示
        // 注意：tmp435_temp_offset 可能是處理 -64C 偏移的機制
        // 這裡的轉換是基於 TMP435 模板的簡化處理，只儲存整數攝氏度
        // 如果需要 0.0625C 解析度，此處需要更精確的轉換。
        
        // TMP435 的 FRACTION_INC (0x80) 代表 0.5C [11]，這裡用於四捨五入到整數
        if ( (value_l >> 4) >= (TMP451_FRACTION_INC >> 4) ) { // 比較低位元組的實際值 (LT3-LT0)
            data->temp_local = ((int32_t)value_h) + 1; // 向上取整
        } else {
            data->temp_local = (int32_t)value_h;
        }
        
        // 如果採用了擴展範圍 (-64C 到 191C)，並且數據是 -64C 偏移的無符號數，則需要調整
        // 這是因為 TMP435 的 tmp435_temp_offset = -64，用於將原始讀數轉換為實際溫度 [11]。
        // TMP451 的擴展範圍也使用 -64C 偏移 [48]。
        // 所以，如果讀取的是擴展範圍的數據，例如讀到 0x40h (64)，實際是 0C，
        // 則需要 64 - 64 = 0。讀到 0x00h (0)，實際是 -64C，則需要 0 - 64 = -64。
        // 這意味著在賦值給 data->temp_local 之前，需要先進行處理。
        // 為簡化起見，如果 TMP451 的配置為擴展範圍，則需要將 temp_raw 解碼後再減去 64。
        // 如果 `value_h` 已經是經過 `RANGE` 位元調整的，那麼直接使用 `value_h` 並應用 `tmp451_temp_offset` 是正確的。
        // 這裡沿用 TMP435 的處理方式，假定 `tmp451_temp_offset` 已經應用到整數部分。
        data->temp_local += tmp451_temp_offset; // 應用偏移量以獲取實際溫度

        LOG_DBG("Local Temp H:0x%02x, L:0x%02x, Raw: %d, Converted: %dC", value_h, value_l, temp_raw, data->temp_local);
    }

    // --- 讀取遠端溫度 ---
    if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_AMBIENT_TEMP)) {
        // TMP451 並不像 TMP435 那樣有一個明確的 "external_channel" 開關。
        // 遠端感測器是否連接可以通過狀態暫存器的 OPEN 位元 (Bit 2) 檢查 [35, 52]。
        // 如果 D+ 和 D- 輸入未連接，應將其連接在一起以防止無意義的故障警告 [52, 53]。
        // 因此，此處的 `cfg->external_channel` 判斷在 TMP451 驅動中可能不再適用或需要調整其含義。
        // 如果您始終使用遠端感測器，可以移除 `if (!(cfg->external_channel))` 判斷。
        // 如果需要檢查是否開路，可以在此處讀取狀態暫存器並檢查 OPEN 位。
        
        // 讀取高位元組，再讀取低位元組 [24, 26, 27]
        ret = tmp451_reg_read(cfg, TMP451_REMOTE_TEMP_H_REG, &value_h, sizeof(value_h));
        if (ret < 0) {
            LOG_ERR("Failed to read TMP451_REMOTE_TEMP_H_REG ret:%d", ret);
            return ret;
        }

        ret = tmp451_reg_read(cfg, TMP451_REMOTE_TEMP_L_REG, &value_l, sizeof(value_l));
        if (ret < 0) {
            LOG_ERR("Failed to read TMP451_REMOTE_TEMP_L_REG, ret:%d", ret);
            return ret;
        }

        temp_raw = ((int32_t)value_h << 4) | ((value_l >> 4) & 0x0F);
        
        if ( (value_l >> 4) >= (TMP451_FRACTION_INC >> 4) ) {
            data->temp_remote = ((int32_t)value_h) + 1;
        } else {
            data->temp_remote = (int32_t)value_h;
        }
        data->temp_remote += tmp451_temp_offset;

        LOG_DBG("Remote Temp H:0x%02x, L:0x%02x, Raw: %d, Converted: %dC", value_h, value_l, temp_raw, data->temp_remote);
    }

    return 0;
}

static int tmp451_channel_get(const struct device *dev, enum sensor_channel chan,
                              struct sensor_value *val)
{
    int ret = 0;
    struct tmp451_data *data = dev->data;
    const struct tmp451_config *cfg = dev->config;

    switch (chan) {
    case SENSOR_CHAN_DIE_TEMP: // 對應本地溫度
        val->val1 = data->temp_local;
        // 注意：如果需要 0.0625°C 解析度，val2 應當被計算填充
        // TMP435 模板中 val2 始終為 0，這意味著它只提供整數攝氏度。
        // 例如：要獲得 0.0625°C 的 val2，需要將低位元組的讀數轉換
        // val->val2 = (data->low_byte_raw_local >> 4) * 62500; // 假設 low_byte_raw_local 被儲存
        val->val2 = 0; // 沿用 TMP435 模板的行為，只提供整數精度
        break;

    case SENSOR_CHAN_AMBIENT_TEMP: // 對應遠端溫度
        // 由於 TMP451 沒有明確的 external_channel boolean 配置，您可以選擇
        // 1. 如果遠端感測器未連接，直接返回 -ENOTSUP。
        // 2. 始終嘗試讀取遠端感測器，並在應用層檢查狀態暫存器的 OPEN 位。
        // 這裡沿用 TMP435 的 `external_channel` 邏輯，但需要通過 DTS 設置此配置。
        // 實際在 TMP451 上，這應該更多地由狀態暫存器中的 OPEN 位元 [35] 來指示。
        // 如果遠端感測器 D+ 和 D- 未連接，其必須連接在一起 [52]。
        if (cfg->alert_therm2_mode) { // 這裡使用 alert_therm2_mode 作為示例判斷，實際應根據您的設計
                                    // 是否始終啟用遠端感測器或根據狀態寄存器 OPEN 位判斷
            val->val1 = data->temp_remote;
            val->val2 = 0; // 沿用 TMP435 模板的行為
        } else {
            ret = -ENOTSUP; // 如果配置上未啟用或檢測到開路
        }
        break;

    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}

static const struct sensor_driver_api tmp451_driver_api = {
	.sample_fetch = tmp451_sample_fetch,
	.channel_get = tmp451_channel_get
};

static int tmp451_init(const struct device *dev)
{
    uint8_t data = 0;
    int ret = 0;
    const struct tmp451_config *cfg = dev->config;

    if (!(i2c_is_ready_dt(&cfg->i2c))) {
        LOG_ERR("I2C dev not ready");
        return -ENODEV;
    }

    // --- 軟體重置 (與 TMP435 不同) ---
    // TMP451 使用通用呼叫位址 00h，然後第二個位元組為 06h 執行軟體重置
    uint8_t reset_cmd[] = {0x06};
    ret = i2c_write_dt(&cfg->i2c, reset_cmd, sizeof(reset_cmd)); // 通用呼叫不需要設備位址
    if (ret < 0) {
        LOG_ERR("Failed to send General Call Reset ret:%d", ret);
        return ret;
    }
    k_msleep(10); // 等待重置完成

    // --- 驗證製造商 ID (可選，但建議) ---
    uint8_t manufacturer_id;
    ret = tmp451_reg_read(cfg, TMP451_MANUFACTURER_ID_REG, &manufacturer_id, 1);
    if (ret < 0) {
        LOG_ERR("Failed to read Manufacturer ID ret:%d", ret);
        return ret;
    }
    if (manufacturer_id != TMP451_MANUFACTURER_ID_VALUE) {
        LOG_ERR("Invalid Manufacturer ID: 0x%02x, expected 0x%02x",
                manufacturer_id, TMP451_MANUFACTURER_ID_VALUE);
        return -ENODEV;
    }
    LOG_DBG("Manufacturer ID: 0x%02x (TMP451 confirmed)", manufacturer_id);

    // --- 配置暫存器 (讀寫位址不同，位元定義不同) ---
    // 預設配置: 啟用 ALERT 輸出, 連續轉換模式, 0C~127C 範圍
    data = 0; // 預設值為 00h
    if (cfg->alert_therm2_mode) {
        data |= TMP451_CONFIG_ALERT_THERM2_MASK; // 設定為 THERM2 模式
    }
    // 其他配置，例如關機、測量範圍等，可根據 cfg 參數或預設值設置
    // 例如：如果需要設定為連續轉換模式 (SD=0，預設值，所以不需要設置)
    // 例如：如果需要設定為擴展範圍 (-64C~191C)，則 data |= TMP451_CONFIG_RANGE_MASK;
    // 這裡我們預設為 0C~127C 範圍 (RANGE=0)
    ret = tmp451_reg_write(cfg, TMP451_CONFIG_REG_WRITE, &data, 1); // 使用寫入位址 0x09
    if (ret < 0) {
        LOG_ERR("Failed to write TMP451_CONFIG_REG_WRITE ret:%d", ret);
        return ret;
    }

    // --- 轉換率暫存器 ---
    // 預設為 16 conversions/sec (0x08)
    data = cfg->conversion_rate; // 從 DT 或 Kconfig 獲取
    ret = tmp451_reg_write(cfg, TMP451_CONVERSION_RATE_REG_WRITE, &data, 1); // 使用寫入位址 0x0A
    if (ret < 0) {
        LOG_ERR("Failed to write TMP451_CONVERSION_RATE_REG_WRITE ret:%d", ret);
        return ret;
    }

    // --- 數位濾波器控制暫存器 ---
    // 預設為關閉 (0x00)
    data = cfg->digital_filter_level; // 從 DT 或 Kconfig 獲取
    ret = tmp451_reg_write(cfg, TMP451_DIGITAL_FILTER_REG_WRITE, &data, 1);
    if (ret < 0) {
        LOG_ERR("Failed to write TMP451_DIGITAL_FILTER_REG_WRITE ret:%d", ret);
        return ret;
    }

    // --- η-因數校正暫存器 (預設 00h 為 1.008) ---
    // 如果需要調整，可以從 cfg 中獲取值並寫入
    // ret = tmp451_reg_write(cfg, TMP451_ETA_FACTOR_CORRECTION_REG, &cfg->eta_factor_correction_val, 1); [29]

    // --- 遠端溫度偏移暫存器 (如果需要初始化) ---
    // ret = tmp451_reg_write(cfg, TMP451_REMOTE_OFFSET_H_REG, &high_byte_offset, 1); [43]
    // ret = tmp451_reg_write(cfg, TMP451_REMOTE_OFFSET_L_REG, &low_byte_offset, 1); [44]

    return 0;
}

/*
 * Device creation macros
 */

#define TMP451_INST(inst) \
    static struct tmp451_data tmp451_data_##inst; \
    static const struct tmp451_config tmp451_config_##inst = { \
        .i2c = I2C_DT_SPEC_INST_GET(inst), \
        .alert_therm2_mode = DT_INST_PROP(inst, alert_therm2_mode), \
        .conversion_rate = DT_INST_PROP(inst, conversion_rate), \
        .digital_filter_level = DT_INST_PROP(inst, digital_filter_level), \
        /* .eta_factor_correction_val = DT_INST_PROP(inst, eta_factor_correction_val), */ \
        /* .remote_offset_celsius = DT_INST_PROP(inst, remote_offset_celsius), */ \
    }; \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, tmp451_init, NULL, &tmp451_data_##inst, \
                                 &tmp451_config_##inst, POST_KERNEL, \
                                 CONFIG_SENSOR_INIT_PRIORITY, &tmp451_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMP451_INST)
