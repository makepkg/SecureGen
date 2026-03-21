#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <RTClib.h>
#include <Wire.h>

static constexpr int RTC_BOOT_COMPENSATION_SEC = 0;

struct RTCConfig {
  bool enabled = false;
  int sda_pin = 21;
  int scl_pin = 22;
};

class RTCManager {
public:
  bool loadConfig();
  bool saveConfig(const RTCConfig &cfg);
  bool init();
  bool reinit();
  bool syncFromRTC();
  bool syncToRTC(time_t t);
  bool isAvailable() const { return _available; }
  bool isInitialized() const { return _initialized; }
  RTCConfig getConfig() const { return _config; }
  DateTime getCurrentTime();

private:
  RTC_DS3231 _rtc;
  RTCConfig _config;
  bool _initialized = false;
  bool _available = false;
};
