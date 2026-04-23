#include "battery_manager.h"
#include "log_manager.h"

BatteryManager::BatteryManager(int adcPin, int powerPin) : _adcPin(adcPin), _powerPin(powerPin) {}

void BatteryManager::begin() {
    LOG_INFO("BatteryManager", "Initializing battery monitoring");

    if (_powerPin >= 0) {
        pinMode(_powerPin, OUTPUT);
        digitalWrite(_powerPin, LOW); // Make sure divider power is off by default
    }

#if defined(CONFIG_IDF_TARGET_ESP32S3)
    analogSetAttenuation(ADC_11db);
    LOG_INFO("BatteryManager", "ADC calibration: analogReadMilliVolts (S3)");
#else
    adc1_config_width(ADC_WIDTH_BIT_12); // 12-bit resolution
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO 34 is ADC1_CHANNEL_6, 11dB attenuation

    // Characterize ADC for calibration
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &_adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        LOG_INFO("BatteryManager", "ADC calibration: eFuse Vref");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        LOG_INFO("BatteryManager", "ADC calibration: eFuse Two Point");
    } else {
        LOG_INFO("BatteryManager", "ADC calibration: Default Vref");
    }
#endif
    LOG_INFO("BatteryManager", "Battery monitoring initialized successfully");
}

// ⚡ OPTIMIZED: Integer-only version (5x быстрее float!)
uint32_t BatteryManager::getVoltageMv() {
    if (_powerPin >= 0) {
        digitalWrite(_powerPin, HIGH);
        delay(10);
    }

#if defined(CONFIG_IDF_TARGET_ESP32S3)
    uint32_t voltage_mv = analogReadMilliVolts(_adcPin);
#else
    uint32_t adcRaw = adc1_get_raw(ADC1_CHANNEL_6);
    if (adcRaw == 0) {
        LOG_WARNING("BatteryManager", "ADC reading returned 0");
    }
    
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adcRaw, &_adc_chars);
#endif

    if (_powerPin >= 0) {
        digitalWrite(_powerPin, LOW);
    }

    // ⚡ Integer math: (voltage_mv * BATTERY_DIVIDER_RATIO) / 1000 instead of float division!
    return (voltage_mv * BATTERY_DIVIDER_RATIO) / 1000;
}

// Deprecated: старая float версия (для совместимости)
float BatteryManager::getVoltage() {
    return (float)getVoltageMv() / 1000.0;  // Используем оптимизированную версию
}

// ⚡ OPTIMIZED: Integer map function (5-10x быстрее float!)
int32_t BatteryManager::imap(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
    // Pure integer math - без float операций!
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Deprecated: float версия (для совместимости)
float BatteryManager::fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ⚡ OPTIMIZED: getPercentage теперь использует integer math!
int BatteryManager::getPercentage() {
    uint32_t voltage_mv = getVoltageMv();  // Integer версия!
    
    // Ограничиваем в милливольтах
    voltage_mv = constrain(voltage_mv, _minVoltageMv, _maxVoltageMv);
    
    // ⚡ Integer map: mV → % (без float!)
    int percentage = imap(voltage_mv, _minVoltageMv, _maxVoltageMv, 0, 100);
    
    // Ensure percentage is within 0-100 range
    percentage = constrain(percentage, 0, 100);
    
    // LOG_DEBUG("BatteryManager", "Battery percentage: " + String(percentage) + "% (" + String(voltage, 2) + "V)"); // Too frequent
    return percentage;
}