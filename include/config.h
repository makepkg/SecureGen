#ifndef CONFIG_H
#define CONFIG_H

// Веб-сервер
#define WEB_SERVER_PORT 80
#define SESSION_TIMEOUT 900000 // 15 минут в миллисекундах

// ============================================================
// Board-specific hardware configuration
// ============================================================
#if defined(BOARD_T_DISPLAY_S3)

// T-Display S3: ESP32-S3, 170x320 ST7789 via 8-bit parallel
// TFT pins are configured via build_flags for TFT_eSPI
// BUTTON_1 (0) and BUTTON_2 (14) are defined by the board variant pins_arduino.h
#define PIN_POWER_ON 15    // GPIO 15: LCD + battery power enable (must be HIGH)
#define TFT_BL   38
#define BATTERY_ADC_PIN    4
#define BATTERY_POWER_PIN -1   // Divider always active on S3
#define BATTERY_DIVIDER_RATIO 2000  // 2.0x (100K+100K divider), integer *1000
#define WAKEUP_BUTTON_NUM  14

#else

// Original T-Display: ESP32, 135x240 ST7789 via SPI
// Display - correct pins for T-Display
#define TFT_WIDTH 135
#define TFT_HEIGHT 240
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  23
#define TFT_BL   4
#define SPI_FREQUENCY 27000000
// Buttons for T-Display
#define BUTTON_1 35
#define BUTTON_2 0
#define BATTERY_ADC_PIN    34
#define BATTERY_POWER_PIN  14
#define BATTERY_DIVIDER_RATIO 1826  // 1.826x divider, integer *1000
#define WAKEUP_BUTTON_NUM  0

#endif

// TOTP настройки
#define CONFIG_TOTP_STEP_SIZE 30
#define CONFIG_TOTP_DIGITS 6

// Файловая система
#define KEYS_FILE "/keys.json.enc"
#define PASSWORD_FILE "/passwords.json.enc"
#define WIFI_CONFIG_FILE "/wifi_config.json.enc"  // Зашифрованный файл WiFi credentials
#define WIFI_CONFIG_FILE_LEGACY "/wifi_config.json"  // Старый plain text файл для миграции
#define SPLASH_IMAGE_PATH "/splash.raw"
#define AUTH_FILE "/auth.json"
#define WIFI_PASSWORD "Your_Password"

// BLE Settings
#define BLE_DEVICE_NAME "T-Disp-Password"
#define BLE_MANUFACTURER_NAME "T-Disp"

// --- PIN Settings ---
#define PIN_FILE "/pin_config.json"
#define PIN_ATTEMPTS_FILE "/.pin_attempts"
#define DEFAULT_PIN_LENGTH 6

// --- General Config ---
#define CONFIG_FILE "/config.json"
#define THEME_CONFIG_KEY "theme"

// --- BLE Config ---
#define BLE_CONFIG_FILE "/ble_config.json"
#define DEFAULT_BLE_DEVICE_NAME "T-Disp-TOTP"

// --- BLE PIN Generation ---
#define BLE_PIN_MIN_VALUE 100000      // 6-значный PIN минимум
#define BLE_PIN_MAX_VALUE 999999      // 6-значный PIN максимум 
#define BLE_PIN_LENGTH 6              // Длина PIN кода
#define BLE_PIN_AUTO_GENERATE true    // Автоматическая генерация при первом запуске

// --- mDNS Config ---
#define MDNS_CONFIG_FILE "/mdns_config.json"
#define DEFAULT_MDNS_HOSTNAME "t-disp-totp"

// --- PBKDF2 Security Settings ---
// Балансировка безопасности и производительности для ESP32 @ 240MHz
// OWASP рекомендует 600,000+ для серверов, но это слишком медленно для ESP32
// 
// ⚠️ ВАЖНО: Реальная производительность mbedTLS на ESP32 (software SHA256):
// 10,000 iterations  ≈ 2 секунды (тестировано)
// 50,000 iterations  ≈ 10 секунд → WATCHDOG TIMEOUT!
// 
// Выбраны максимальные значения, которые НЕ блокируют async tasks:

#define PBKDF2_ITERATIONS_PIN 25000      // device key unlock, once per boot (~2.7s)
#define PBKDF2_ITERATIONS_LOGIN 25000    // web login, each authentication
#define PBKDF2_ITERATIONS_EXPORT 15000   // import/export, rare operation (~1.6s)

// Производительность на ESP32 @ 240MHz (реальные измерения):
// 10,000 iterations  ≈ 2 секунды (приемлемо для login, не блокирует watchdog)
// 15,000 iterations  ≈ 3 секунды (приемлемо для редких операций)


#endif

