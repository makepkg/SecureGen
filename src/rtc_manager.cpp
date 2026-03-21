#include "rtc_manager.h"
#include "log_manager.h"

static const char *TAG = "RTC";
static const char *RTC_CONFIG_FILE = "/rtc_config.json";
static const time_t RTC_VALID_THRESHOLD = 1609459200; // Jan 1, 2021

bool RTCManager::loadConfig() {
  if (!LittleFS.exists(RTC_CONFIG_FILE))
    return false;
  File f = LittleFS.open(RTC_CONFIG_FILE, "r");
  if (!f)
    return false;
  JsonDocument doc;
  if (deserializeJson(doc, f) != DeserializationError::Ok) {
    f.close();
    return false;
  }
  f.close();
  _config.enabled = doc["enabled"] | false;
  _config.sda_pin = doc["sda_pin"] | 21;
  _config.scl_pin = doc["scl_pin"] | 22;
  return true;
}

bool RTCManager::saveConfig(const RTCConfig &cfg) {
  _config = cfg;
  File f = LittleFS.open(RTC_CONFIG_FILE, "w");
  if (!f)
    return false;
  JsonDocument doc;
  doc["enabled"] = cfg.enabled;
  doc["sda_pin"] = cfg.sda_pin;
  doc["scl_pin"] = cfg.scl_pin;
  serializeJson(doc, f);
  f.close();
  return true;
}

bool RTCManager::init() {
  if (_initialized)
    return _available; // guard: Wire.begin only once
  Wire.begin(_config.sda_pin, _config.scl_pin);
  _initialized = true;
  if (!_rtc.begin(&Wire)) {
    LOG_ERROR(TAG,
              "DS3231 not found on I2C bus (SDA=" + String(_config.sda_pin) +
                  " SCL=" + String(_config.scl_pin) + ")");
    _available = false;
    return false;
  }
  _available = true;
  LOG_INFO(TAG, "DS3231 initialized (SDA=" + String(_config.sda_pin) +
                    " SCL=" + String(_config.scl_pin) + ")");
  return true;
}

bool RTCManager::reinit() {
  _initialized = false;
  _available = false;
  return init();
}

bool RTCManager::syncFromRTC() {
  if (!_available)
    return false;
  DateTime now = _rtc.now();
  time_t ts = now.unixtime();
  if (ts < RTC_VALID_THRESHOLD) {
    LOG_WARNING(TAG, "DS3231 time invalid (ts=" + String((unsigned long)ts) +
                         ") — battery dead or never set");
    return false;
  }
  struct timeval tv = {ts, 0};
  settimeofday(&tv, nullptr);
  LOG_INFO(TAG, "System time set from DS3231: " + String((unsigned long)ts));
  return true;
}

bool RTCManager::syncToRTC(time_t t) {
  if (!_available)
    return false;
  // Always wait for second boundary — gives DS3231 a clean whole-second value
  struct timeval tv_now;
  gettimeofday(&tv_now, nullptr);
  // Busy-wait for actual second rollover — most accurate method
  // We watch tv_sec change, then immediately write the new value
  struct timeval tv_snap;
  gettimeofday(&tv_snap, nullptr);
  time_t sec_before = tv_snap.tv_sec;
  do {
    gettimeofday(&tv_snap, nullptr);
  } while (tv_snap.tv_sec == sec_before);
  // tv_snap.tv_sec just incremented — this is the cleanest second boundary
  t = tv_snap.tv_sec;
  _rtc.adjust(DateTime((uint32_t)(t + RTC_BOOT_COMPENSATION_SEC)));
  LOG_INFO(TAG, "DS3231 updated with ts=" + String((unsigned long)t) + 
           " (compensation +" + String(RTC_BOOT_COMPENSATION_SEC) + "s applied)");
  return true;
}

DateTime RTCManager::getCurrentTime() { return _rtc.now(); }
