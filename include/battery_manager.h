#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include "config.h"

#if defined(CONFIG_IDF_TARGET_ESP32)
#include "esp_adc_cal.h" // Required for ADC calibration
#include "driver/adc.h"  // Required for ADC driver functions
#endif

class BatteryManager {
public:
    BatteryManager(int adcPin, int powerPin);
    void begin();
    
    // ⚡ OPTIMIZED: Возвращаем милливольты (integer) вместо float вольтов
    uint32_t getVoltageMv();  // 5x быстрее чем getVoltage()
    float getVoltage();        // Deprecated: оставляем для совместимости
    int getPercentage();

private:
    int _adcPin;
    int _powerPin;
    // ⚡ OPTIMIZED: Калибровка в милливольтах (integer math)
    const uint32_t _maxVoltageMv = 3800;  // 3.8V = 3800mV
    const uint32_t _minVoltageMv = 3200;  // 3.2V = 3200mV
    
    // Deprecated float версии (для совместимости)
    const float _maxVoltage = 3.8;
    const float _minVoltage = 3.2;

#if defined(CONFIG_IDF_TARGET_ESP32)
    esp_adc_cal_characteristics_t _adc_chars; // ADC calibration characteristics
#endif

    // ⚡ OPTIMIZED: Integer map function (5-10x быстрее float)
    int32_t imap(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
    
    // Deprecated: float версия (оставляем для совместимости)
    float fmap(float x, float in_min, float in_max, float out_min, float out_max);
};

#endif // BATTERY_MANAGER_H
