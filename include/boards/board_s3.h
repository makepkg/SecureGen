#pragma once

// ============================================================
// Board: LilyGo T-Display S3
// MCU: ESP32-S3R8 LX7 | Display: ST7789V 8-bit parallel 170x320
// Flash: 16MB | PSRAM: 8MB | USB: Native CDC
// ============================================================

// BUTTON_1 and BUTTON_2 are defined by the board variant pins_arduino.h
// variants/lilygo_t_display_s3/pins_arduino.h: BUTTON_1=14, BUTTON_2=0

// --- Battery ADC ---
// GPIO4 = ADC1_CHANNEL_3 on ESP32-S3 (LCD_BAT_VOLT pin)
#define BATTERY_ADC_PIN   4
#define BATTERY_ADC_CH    ADC1_CHANNEL_3
#define BATTERY_POWER_PIN -1    // No voltage divider power pin on S3
#define BATTERY_DIVIDER_RATIO 1890  // measured: 4.1V bat / 2.17V ADC = 1.89x

// --- LCD Power ---
// Must be driven HIGH before tft.init()
#define LCD_POWER_ON_PIN 15

// --- I2C defaults (user-configurable via /rtc_config.json) ---
// GPIO22 is not available on T-Display S3!
// Default: SDA=21, SCL=16
#define DEFAULT_I2C_SDA 21
#define DEFAULT_I2C_SCL 16

// --- Deep sleep wakeup ---
#define WAKEUP_GPIO GPIO_NUM_0

// --- HID Loader Text ---
#define HID_LOADER_TEXT "Activating BLE/USB..."

// TFT pins are configured via platformio.ini build flags.
