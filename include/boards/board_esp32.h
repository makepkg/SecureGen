#pragma once

// ============================================================
// Board: LilyGo T-Display ESP32 (original)
// MCU: ESP32 LX6 | Display: ST7789 SPI 135x240 | Flash: 16MB
// ============================================================

// --- Buttons ---
#define BUTTON_1 35
#define BUTTON_2 0

// --- Battery ADC ---
// GPIO34 = ADC1_CHANNEL_6 on ESP32
#define BATTERY_ADC_PIN   34
#define BATTERY_ADC_CH    ADC1_CHANNEL_6
#define BATTERY_POWER_PIN 14
#define BATTERY_DIVIDER_RATIO 1826  // original measured ratio

// --- I2C defaults (user-configurable via /rtc_config.json) ---
#define DEFAULT_I2C_SDA 21
#define DEFAULT_I2C_SCL 22

// --- Deep sleep wakeup ---
#define WAKEUP_GPIO GPIO_NUM_0

// --- HID Loader Text ---
#define HID_LOADER_TEXT "Activating BLE..."

// TFT pins are configured via platformio.ini build flags.
// No LCD_POWER_ON_PIN on this board.
