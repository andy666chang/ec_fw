/*
 * Copyright (c) 2024 Bittium Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP451_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP451_H_

// --- 1. I2C 從屬位址 ---
#define TMP451_I2C_ADDR                     0x4C

// --- 2. 溫度資料暫存器 (與 TMP435 相同) ---
#define TMP451_LOCAL_TEMP_H_REG             0x00
#define TMP451_REMOTE_TEMP_H_REG            0x01
#define TMP451_REMOTE_TEMP_L_REG            0x10 // 高4位元有效
#define TMP451_LOCAL_TEMP_L_REG             0x15 // 高4位元有效

// --- 3. 狀態暫存器 (與 TMP435 位址相同，位元定義需擴展) ---
#define TMP451_STATUS_REG                   0x02
#define TMP451_STATUS_REG_BUSY              0x80 // Bit 7: ADC 轉換忙碌
// 新增 TMP451 狀態位元定義 [32-36]
#define TMP451_STATUS_REG_LHIGH             0x40 // Bit 6: 本地溫度超過高限
#define TMP451_STATUS_REG_LLOW              0x20 // Bit 5: 本地溫度低於低限
#define TMP451_STATUS_REG_RHIGH             0x10 // Bit 4: 遠端溫度超過高限
#define TMP451_STATUS_REG_RLOW              0x08 // Bit 3: 遠端溫度低於低限
#define TMP451_STATUS_REG_OPEN              0x04 // Bit 2: 遠端接點開路
#define TMP451_STATUS_REG_RTHRM             0x02 // Bit 1: 遠端 THERM 限值觸發
#define TMP451_STATUS_REG_LTHRM             0x01 // Bit 0: 本地 THERM 限值觸發

// --- 4. 配置暫存器 (讀寫位址不同，位元定義需調整) ---
#define TMP451_CONFIG_REG_READ              0x03
#define TMP451_CONFIG_REG_WRITE             0x09
#define TMP451_CONFIG_MASK1_MASK            0x80 // Bit 7: ALERT 輸出遮罩
#define TMP451_CONFIG_SD_MASK               0x40 // Bit 6: 關機控制 (1=關機, 0=連續轉換)
#define TMP451_CONFIG_ALERT_THERM2_MASK     0x20 // Bit 5: ALERT 或 THERM2 模式選擇 (1=THERM2, 0=ALERT)
#define TMP451_CONFIG_RANGE_MASK            0x04 // Bit 2: 溫度測量範圍 (1=-64C~191C, 0=0C~127C)

// --- 5. 轉換率暫存器 (TMP435 無此明確暫存器) ---
#define TMP451_CONVERSION_RATE_REG_READ     0x04
#define TMP451_CONVERSION_RATE_REG_WRITE    0x0A
#define TMP451_CONVERSION_RATE_DEFAULT      0x08 // 預設 16 conversions/sec

// --- 6. 單次轉換啟動暫存器 (與 TMP435 相同) ---
#define TMP451_ONE_SHOT_START_REG           0x0F

// --- 7. η-因數校正暫存器 (取代 TMP435 的 Beta Compensation) ---
#define TMP451_ETA_FACTOR_CORRECTION_REG    0x23
// η-因數預設值在寫入 00h 時為 1.008 [42]

// --- 8. 遠端溫度偏移暫存器 (TMP435 可能通過其他方式實現) ---
#define TMP451_REMOTE_OFFSET_H_REG          0x11
#define TMP451_REMOTE_OFFSET_L_REG          0x12

// --- 9. 數位濾波器控制暫存器 (TMP435 無此明確暫存器) ---
#define TMP451_DIGITAL_FILTER_REG_READ      0x24
#define TMP451_DIGITAL_FILTER_REG_WRITE     0x24
#define TMP451_DIGITAL_FILTER_OFF           0x00 // 預設關閉
#define TMP451_DIGITAL_FILTER_LEVEL1        0x01 // 4次採樣移動平均
#define TMP451_DIGITAL_FILTER_LEVEL2        0x02 // 8次採樣移動平均

// --- 10. 製造商 ID 暫存器 (用於設備驗證) ---
#define TMP451_MANUFACTURER_ID_REG          0xFE
#define TMP451_MANUFACTURER_ID_VALUE        0x55

// --- 11. 其他常數 (沿用 TMP435 的) ---
#define TMP451_CONV_LOOP_LIMIT              50   /* 建議的忙等待循環限制 [11] */
#define TMP451_FRACTION_INC                 0x80 /* TMP435 模板中 0.5°C 的低位元組表示 [11]。對於 TMP451，低位元組的位元 7-4 決定小數部分，0x80 代表 0.5000°C [27, 47]。*/

// --- 12. 溫度偏移 (用於處理擴展溫度範圍，來自 TMP435 模板) ---
// TMP451 的擴展二進位格式將 -64C 映射到 0x00 [48]。所以轉換時需要減去 64。
static const int32_t tmp451_temp_offset = -64;

// --- 13. 結構體定義 ---
struct tmp451_data {
    int32_t temp_local;   /* 本地溫度，單位攝氏度 (基於TMP435模板，此處為整數部分) */
    int32_t temp_remote;  /* 遠端溫度，單位攝氏度 (基於TMP435模板，此處為整數部分) */
};

struct tmp451_config {
    struct i2c_dt_spec i2c;
    // 根據 TMP451 的功能，定義其可配置項
    bool alert_therm2_mode;    // true: THERM2 mode, false: ALERT mode [38]
    uint8_t conversion_rate;   // 轉換率設定值 [40]
    uint8_t digital_filter_level; // 數位濾波器等級 [45]
    // 添加 η-factor 和 remote offset 的初始化值如果需要預設配置
    // uint8_t eta_factor_correction_val;
    // int16_t remote_offset_celsius; // 如果需要初始化偏移量
};

#endif /* ZEPHYR_DRIVERS_SENSOR_TMP451_H_ */
