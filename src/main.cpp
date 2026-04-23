#include "LittleFS.h"
#include "PasswordManager.h"
#include "app_modes.h" // Используем новый общий заголовок
#include "battery_manager.h"
#include "ble_keyboard_manager.h"
#include "config.h"
#include "config_manager.h"
#include "crypto_manager.h"
#include "display_manager.h"
// esp_sleep.h removed — pseudo-sleep uses polling loop, no esp_sleep_* calls needed
#include "log_manager.h"
#include "pin_manager.h"
#include "rtc_manager.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include "splash_manager.h"
#include "totp_generator.h"
#include "web_admin_manager.h"
#include "web_server.h"
#include <Arduino.h>
#include <ESPmDNS.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>

#ifdef SECURE_LAYER_ENABLED
#include "secure_layer_manager.h"
#include "traffic_obfuscation_manager.h"
#include "url_obfuscation_manager.h"
#endif

// Global flag for device restart
bool shouldRestart = false;

// Global flag for PIN disable request (set by web server)
bool shouldPromptPinDisable = false;

// Pending theme change (set by web server, applied in main loop to avoid watchdog)
bool pendingThemeChange = false;
Theme pendingTheme = Theme::LIGHT;

#define WDT_TIMEOUT 10

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Глобальные объекты менеджеров
DisplayManager displayManager;
ConfigManager configManager;
KeyManager keyManager;
PasswordManager passwordManager;
SplashScreenManager splashManager(displayManager);
PinManager pinManager(displayManager);
BatteryManager batteryManager(34, 14);
BleKeyboardManager bleKeyboardManager(DEFAULT_BLE_DEVICE_NAME, "Lord", 100);
WifiManager wifiManager(displayManager, configManager);
TOTPGenerator totpGenerator;
RTCManager rtcManager;
WebServerManager webServerManager(keyManager, splashManager, displayManager,
                                  pinManager, configManager, passwordManager,
                                  totpGenerator);

#ifdef SECURE_LAYER_ENABLED
SecureLayerManager &secureLayerManager = SecureLayerManager::getInstance();
TrafficObfuscationManager &trafficObfuscationManager =
    TrafficObfuscationManager::getInstance();
#endif

// Secure shutdown function - wipes sensitive data before deep sleep
void secureShutdown() {
  LOG_INFO("Main", "Secure shutdown: wiping sensitive data...");
  CryptoManager::getInstance().wipeDeviceKey();
  keyManager.wipeSecrets();
  passwordManager.wipePasswords();
#ifdef SECURE_LAYER_ENABLED
  secureLayerManager.wipeAllSessions();
#endif
  delay(50);
  LOG_INFO("Main", "Secure shutdown: entering deep sleep");
}

// PIN attempt counter persistence functions
int loadPinAttempts() {
  if (!LittleFS.exists(PIN_ATTEMPTS_FILE))
    return 0;
  fs::File f = LittleFS.open(PIN_ATTEMPTS_FILE, "r");
  if (!f)
    return 0;
  int count = f.parseInt();
  f.close();
  return count;
}

void savePinAttempts(int count) {
  fs::File f = LittleFS.open(PIN_ATTEMPTS_FILE, "w");
  if (!f)
    return;
  f.print(count);
  f.close();
}

void clearPinAttempts() { LittleFS.remove(PIN_ATTEMPTS_FILE); }

// Глобальные переменные состояния
static AppMode currentMode = AppMode::TOTP;
static int currentKeyIndex = 0;
static int currentPasswordIndex = 0;
static int previousKeyIndex = -1;
static int previousPasswordIndex = -1;
unsigned long lastButtonPressTime = 0;
const int debounceDelay = 300;
const int factoryResetHoldTime = 5000;
const int powerOffHoldTime = 5000;
unsigned long lastActivityTime = 0;
bool isScreenOn = true;

unsigned long bothButtonsPressStartTime = 0;
bool bleActionTriggered = false;
static bool hotpSavePending = false;
static int hotpSaveIndex = -1;

unsigned long lastBatteryCheckTime = 0;
const int batteryCheckInterval = 1000;
static int lastBatteryPercentage = -1;
static int _lastRealBatteryPercentage = 50;
static int chargeDisplayPct = -1;
static bool isCharging = false;

bool configPortalActive = false;
DNSServer adminDnsServer;
StartupMode selectedMode = StartupMode::WIFI_MODE; // Default

unsigned long lastTotpUpdateTime = 0;
const int totpUpdateInterval = 250;

void showWebServerInfoPage() {
  // 🔄 Не вызываем init() - избегаем мигания!
  TFT_eSPI *tft = displayManager.getTft();
  const ThemeColors *colors = displayManager.getCurrentThemeColors();

  // 🌌 Плавное затухание перед отрисовкой
  for (int i = 255; i >= 0; i -= 15) {
    displayManager.setBrightness(i);
    delay(10);
  }

  // Отрисовка при погашенном экране (не видна пользователю)
  tft->fillScreen(colors->background_dark);
  tft->setTextDatum(MC_DATUM); // Middle Center alignment

  // Title
  tft->setTextColor(colors->accent_primary, colors->background_dark);
  tft->setTextSize(2);
  tft->drawString("Web Server Started!", tft->width() / 2, 25);

  // IP Address
  String ip = wifiManager.getIP();
  tft->setTextColor(colors->text_primary, colors->background_dark);
  tft->setTextSize(2);
  tft->drawString(ip, tft->width() / 2, 60);

  // Domain - защита от краша
  tft->setTextColor(colors->accent_secondary, colors->background_dark);
  tft->setTextSize(1);
  String mdnsHostname = configManager.loadMdnsHostname();
  mdnsHostname += ".local";
  tft->drawString(mdnsHostname, tft->width() / 2, 85);

  // Instructions
  tft->setTextColor(colors->text_secondary, colors->background_dark);
  tft->setTextSize(1);
  tft->drawString("Ready for connections", tft->width() / 2, 105);

  // 🌌 Плавное появление
  for (int i = 0; i <= 255; i += 15) {
    displayManager.setBrightness(i);
    delay(10);
  }
  displayManager.turnOn(); // Полная яркость

  delay(3000);

  // 🧹 КРИТИЧНО: Очистка экрана перед возвратом к TOTP
  // Без этого текст "Ready for connections" остается под шкалой!
  tft->fillScreen(colors->background_dark);
}

void handleFactoryResetOnBoot() {
  displayManager.init();
  displayManager.showMessage("Hold both buttons", 10, 20, false, 2);
  displayManager.showMessage("for factory reset.", 10, 40, false, 2);

  unsigned long startTime = millis();

  while (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
    unsigned long holdTime = millis() - startTime;

    if (holdTime > factoryResetHoldTime) {
      displayManager.init();
      displayManager.showMessage("FACTORY RESET!", 10, 30, true, 2);

      LOG_CRITICAL("Main", "--- FACTORY RESET ---");
      LOG_INFO("Main", "Clearing active web sessions...");
      webServerManager.clearSession();
      LOG_INFO("Main", "Deleting files...");
      LittleFS.remove(KEYS_FILE);
      LittleFS.remove("/wifi_config.json");
      LittleFS.remove("/wifi_config.json.enc"); // Зашифрованный WiFi файл
      // SPLASH_IMAGE_PATH removed - custom splash upload disabled for security
      LittleFS.remove(
          "/splash_config.json"); // Splash mode config (reset to disabled)
      LittleFS.remove(PIN_FILE);
      clearPinAttempts();           // Clear PIN attempt counter
      LittleFS.remove(CONFIG_FILE); // Resets display timeout to default (30s) +
                                    // AP password to "12345678"
      LittleFS.remove(DEVICE_KEY_FILE);
      LittleFS.remove(PASSWORD_FILE);
      LittleFS.remove(BLE_CONFIG_FILE);
      LittleFS.remove(WEB_ADMIN_FILE);
      LittleFS.remove(MDNS_CONFIG_FILE);    // <-- СБРОС MDNS
      LittleFS.remove(LOGIN_STATE_FILE);    // <-- СБРОС СОСТОЯНИЯ ЛОГИНА
      LittleFS.remove("/rtc_config.json");
      LittleFS.remove("/ble_pin.json.enc"); // <-- СБРОС BLE PIN
      LittleFS.remove("/session.json.enc"); // <-- СБРОС СЕССИЙ И CSRF

      // 🔗 URL Obfuscation: Удаление boot counter и всех mappings
      LOG_INFO("Main", "Clearing URL obfuscation data...");
      LittleFS.remove("/boot_counter.txt"); // <-- СБРОС BOOT COUNTER

      // 🗑️ Удаляем все url_mappings_*.json файлы
      fs::File root = LittleFS.open("/", "r");
      if (root) {
        fs::File file = root.openNextFile();
        while (file) {
          String filename = String(file.name());
          if (filename.startsWith("/url_mappings_") &&
              filename.endsWith(".json")) {
            LOG_DEBUG("Main", "Removing URL mapping file: " + filename);
            LittleFS.remove(filename);
          }
          file = root.openNextFile();
        }
      }
      LOG_INFO("Main", "URL obfuscation data cleared");

      LOG_INFO("Main", "File deletion complete");

      // КРИТИЧНО: Очистка BLE bonding ключей через NVS partition erase
      LOG_INFO("Main",
               "BLE bonding keys will be cleared by NVS partition erase");

      // Дополнительная очистка NVS BLE раздела
      nvs_flash_erase_partition("nvs");
      LOG_INFO("Main", "NVS partition cleared");

      displayManager.showMessage("Done. Rebooting...", 10, 60);

      delay(2500);
      ESP.restart();
    }

    int progress = (holdTime * 100) / factoryResetHoldTime;
    displayManager.showMessage("Resetting: " + String(progress) + "%", 10, 100);
    delay(100);
  }
  LOG_INFO("Main", "Factory reset aborted. Continuing boot");
  displayManager.init();
}

void setup() {
  Serial.begin(115200);
  LogManager::getInstance().begin();
  LOG_INFO("Main", "T-Disp-TOTP Booting Up");

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  if (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
    if (LittleFS.begin(true)) {
      Theme savedTheme = configManager.loadTheme();
      displayManager.setTheme(savedTheme);
      handleFactoryResetOnBoot();
    } else {
      DisplayManager tempDisplay;
      tempDisplay.init();
      tempDisplay.showMessage("LittleFS Failed!", 10, 30, true);
      while (1)
        ;
    }
  }

  LOG_INFO("Main", "Initializing Battery Manager...");
  batteryManager.begin();
  LOG_INFO("Main", "Initializing LittleFS...");
  if (!LittleFS.begin(true)) {
    LOG_CRITICAL("Main", "LittleFS Mount Failed!");
    DisplayManager tempDisplay;
    tempDisplay.init();
    tempDisplay.showMessage("LittleFS Failed", 10, 30, true);
    while (1)
      ;
  }

  LOG_INFO("Main", "Initializing Crypto Manager...");
  CryptoManager::getInstance().begin();

  // --- НОВАЯ ЛОГИКА: Проверка и создание Device PIN ---
  LOG_INFO("Main", "Checking device key status...");

  if (!CryptoManager::getInstance().isDeviceKeyFileExists()) {
    // ПЕРВАЯ ЗАГРУЗКА: device.key не существует
    LOG_INFO("Main", "First boot detected - no device.key file found");

    displayManager.init();

    // 0. Показываем приветственный экран
    displayManager.showMessage("First Boot!", 10, 20, false, 2);
    displayManager.showMessage("PIN Setup Required", 10, 50, false, 2);
    delay(2000);

    // 1. Выбираем длину PIN
    int pinLength = pinManager.requestPinLengthSelection();
    LOG_INFO("Main", "PIN length selected: " + String(pinLength));

    // 🔋 Сохраняем конфигурацию ПОСЛЕ отпускания кнопки для стабилизации
    // питания Это предотвращает brownout reset на батарее при холодном старте
    pinManager.setPinLength(pinLength);
    delay(50); // Дополнительная задержка для стабилизации питания
    pinManager.saveConfig();
    LOG_INFO("Main", "PIN length configuration saved");

    // 2. Создаем PIN
    bool pinCreated = false;
    while (!pinCreated) {
      pinCreated = pinManager.requestNewPinSetup();
      if (!pinCreated) {
        displayManager.init();
        displayManager.showMessage("PIN Required!", 10, 30, true, 2);
        displayManager.showMessage("Device cannot start", 10, 60, false, 1);
        displayManager.showMessage("without PIN", 10, 75, false, 1);
        delay(2000);
      }
    }

    LOG_INFO("Main", "First boot PIN setup completed successfully");

  } else if (CryptoManager::getInstance().isDeviceKeyEncrypted()) {
    // ОБЫЧНАЯ ЗАГРУЗКА: device.key зашифрован, требуется PIN
    LOG_INFO("Main", "Encrypted device key detected - PIN required");

    displayManager.init();

    bool unlocked = false;
    const int maxAttempts = 5; // Максимум попыток перед блокировкой
    int attempts = loadPinAttempts();

    if (attempts >= maxAttempts) {
      LOG_CRITICAL("Main", "Device locked - max PIN attempts already reached.");
      displayManager.init();
      displayManager.showMessage("DEVICE LOCKED!", 10, 30, true, 2);
      displayManager.showMessage("Too many attempts", 10, 60, false, 1);
      displayManager.showMessage("Factory reset required", 10, 75, false, 1);
      delay(5000);
      secureShutdown();
      esp_deep_sleep_start(); // Блокируем устройство
    }

    while (!unlocked && attempts < maxAttempts) {
      unlocked = pinManager.requestDevicePin();

      if (!unlocked) {
        attempts++;
        savePinAttempts(attempts);
        LOG_WARNING("Main", "Failed PIN attempt " + String(attempts) + "/" +
                                String(maxAttempts));

        if (attempts >= maxAttempts) {
          LOG_CRITICAL("Main", "Maximum PIN attempts exceeded! Device locked.");
          displayManager.init();
          displayManager.showMessage("DEVICE LOCKED!", 10, 30, true, 2);
          displayManager.showMessage("Too many attempts", 10, 60, false, 1);
          displayManager.showMessage("Factory reset required", 10, 75, false,
                                     1);
          delay(5000);
          secureShutdown();
          esp_deep_sleep_start(); // Блокируем устройство
        }

        displayManager.init();
        displayManager.showMessage("Wrong PIN!", 10, 30, true, 2);
        displayManager.showMessage("Attempt " + String(attempts) + "/" +
                                       String(maxAttempts),
                                   10, 60, false, 2);
        delay(1500);
      }
    }

    // PIN correct — clear counter
    clearPinAttempts();
    LOG_INFO("Main", "Device unlocked successfully with PIN");

  } else {
    // LEGACY: device.key существует но не зашифрован (старый формат)
    LOG_WARNING("Main", "Unencrypted device key detected (legacy format)");
    LOG_INFO("Main", "Loading unencrypted key for backward compatibility");

    // Загружаем незашифрованный ключ
    if (!CryptoManager::getInstance().unlockDeviceKeyWithPin("")) {
      LOG_ERROR("Main", "Failed to load legacy unencrypted key");
      displayManager.init();
      displayManager.showMessage("KEY ERROR!", 10, 30, true, 2);
      displayManager.showMessage("Cannot load device key", 10, 60, false, 1);
      delay(3000);
      ESP.restart();
    }

    LOG_INFO(
        "Main",
        "Legacy key loaded. Consider enabling PIN protection in web cabinet.");
  }

#ifdef SECURE_LAYER_ENABLED
  LOG_INFO("Main", "Initializing Secure Layer Manager...");
  if (secureLayerManager.begin()) {
    LOG_INFO("Main", "Secure Layer Manager initialized successfully");
  } else {
    LOG_ERROR("Main", "Failed to initialize Secure Layer Manager");
  }

  // ❌ MOVED: TrafficObfuscation инициализируется в WebServerManager::start()
  // Не должен работать если веб-сервер не запущен!
#endif

  LOG_INFO("Main", "Initializing Web Admin Manager...");
  WebAdminManager::getInstance().begin();

  LOG_INFO("Main", "Loading theme...");
  Theme savedTheme = configManager.loadTheme();
  displayManager.setTheme(savedTheme);

  LOG_INFO("Main", "Loading BLE device name...");
  String savedBleDeviceName = configManager.loadBleDeviceName();
  bleKeyboardManager.setDeviceName(savedBleDeviceName);

  LOG_INFO("Main", "Setting up BLE display manager...");
  bleKeyboardManager.setDisplayManager(&displayManager);

  LOG_INFO("Main", "Setting up web server BLE reference...");
  webServerManager.setBleKeyboardManager(&bleKeyboardManager);
  webServerManager.setWifiManager(&wifiManager);
  webServerManager.setBatteryManager(&batteryManager);

  String startupMode = configManager.getStartupMode();
  LOG_INFO("Main", "Loaded startup mode: " + startupMode);
  if (startupMode == "password") {
    currentMode = AppMode::PASSWORD;
    LOG_INFO("Main", "Starting in Password Manager mode");
  } else {
    currentMode = AppMode::TOTP;
    LOG_INFO("Main", "Starting in TOTP Authenticator mode");
  }

  LOG_INFO("Main", "Initializing Display, Key, Password, and Pin Managers...");
  // Ранняя инициализация для splash (без заполнения экрана и без включения
  // яркости)
  displayManager.initForSplash();
  keyManager.begin();
  passwordManager.begin();
  pinManager.begin();

  LOG_INFO("Main", "Displaying splash screen...");
  splashManager.displaySplashScreen();

  // 🔧 Полная инициализация display после splash
  displayManager.init();

  displayManager.updateMessage("Initializing...", 10, 10, 2);

  // 🌌 ПРОМПТИНГ ВЫБОРА РЕЖИМА (AP/Offline/WiFi)
  LOG_INFO("Main", "Prompting for startup mode...");
  String savedBootMode = configManager.getBootMode();
  StartupMode defaultBootMode = StartupMode::WIFI_MODE;
  if (savedBootMode == "ap") defaultBootMode = StartupMode::AP_MODE;
  else if (savedBootMode == "offline") defaultBootMode = StartupMode::OFFLINE_MODE;
  LOG_INFO("Main", "Default boot mode from config: " + savedBootMode);
  
  // If "Reboot with web server" was requested — skip prompt, force WiFi mode
  if (configManager.getWebServerAutoStart()) {
    LOG_INFO("Main", "Auto-start web server flag set — skipping mode prompt, forcing WiFi mode.");
    selectedMode = StartupMode::WIFI_MODE;
  } else {
    selectedMode = displayManager.promptModeSelection(defaultBootMode);
  }

  // --- DS3231 RTC Init (all modes) ---
  rtcManager.loadConfig();
  if (rtcManager.getConfig().enabled) {
    if (rtcManager.init()) {
      if (rtcManager.syncFromRTC()) {
        LOG_INFO("Main", "System time set from DS3231 RTC");
      } else {
        LOG_WARNING("Main", "DS3231 found but time invalid — sync required");
      }
    } else {
      LOG_WARNING("Main", "DS3231 not found on I2C bus");
    }
  }
  // --- End DS3231 RTC Init ---

  // Переменная для отслеживания синхронизации времени
  struct tm timeinfo;
  bool timeSynced = false;

  if (selectedMode == StartupMode::AP_MODE) {
    // 📡 AP MODE
    LOG_INFO("Main", "User chose AP Mode. Starting Access Point...");

    // If no valid time from RTC, zero out clock so TOTP shows NOT SYNCED
    if (!totpGenerator.isTimeSynced()) {
      struct timeval tv_zero = {0, 0};
      settimeofday(&tv_zero, nullptr);
      LOG_INFO("Main", "AP mode: no valid RTC time, TOTP will show NOT SYNCED");
    }

    // Генерация имени AP на основе MAC
    String apName = "ESP32-TOTP-" + String(WiFi.macAddress().substring(12, 14) +
                                           WiFi.macAddress().substring(15, 17));
    String apPassword = configManager.loadApPassword();

    // Запуск AP точки
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), apPassword.c_str());

    adminDnsServer.start(53, "*", WiFi.softAPIP());
    LOG_INFO("Main", "DNS Server started for Admin AP");

    // Запуск mDNS для AP режима
    String hostname = configManager.loadMdnsHostname();
    if (MDNS.begin(hostname.c_str())) {
      LOG_INFO("Main", "mDNS started in AP mode. Access via: http://" +
                           hostname + ".local");
      MDNS.addService("http", "tcp", 80);
    } else {
      LOG_ERROR("Main", "Failed to start mDNS in AP mode");
    }

    // Отображение информации на экране
    displayManager.init();

    auto drawApInfoScreen = [&]() {
      TFT_eSPI *tft = displayManager.getTft();
      auto *t = displayManager.getCurrentThemeColors();
      int W = tft->width();  // 240
      int H = tft->height(); // 135
      int cx = W / 2;

      tft->fillScreen(t->background_dark);
      tft->setTextDatum(MC_DATUM);

      // Title
      tft->setTextSize(2);
      tft->setTextColor(t->accent_primary, t->background_dark);
      tft->drawString("AP Mode", cx, 18);

      // Divider line
      tft->drawFastHLine(20, 32, W - 40, t->text_secondary);

      // Network name (large, prominent)
      tft->setTextSize(1);
      tft->setTextColor(t->text_secondary, t->background_dark);
      tft->drawString("Network", cx, 44);
      tft->setTextSize(1);
      tft->setTextColor(t->text_primary, t->background_dark);
      tft->drawString(apName, cx, 56);

      // Password and IP on same row area
      tft->setTextColor(t->text_secondary, t->background_dark);
      tft->drawString("Password: " + apPassword +
                          "   IP: " + WiFi.softAPIP().toString(),
                      cx, 70);

      // Button hints — match PIN screen bottom button style
      // BTN1 hint (left side, like AP button in mode selection)
      tft->fillRoundRect(8, H - 52, 106, 22, 6, t->background_light);
      tft->setTextColor(t->text_primary, t->background_light);
      tft->drawString("BTN1: WiFi QR", 8 + 53, H - 41);

      // BTN2 hint (right side, like Offline button in mode selection)
      tft->fillRoundRect(126, H - 52, 106, 22, 6, t->accent_primary);
      tft->setTextColor(t->background_dark, t->accent_primary);
      tft->drawString("BTN2: Continue", 126 + 53, H - 41);
    };

    drawApInfoScreen();

    auto highlightButton = [&](int activeBtn) {
      TFT_eSPI *tft = displayManager.getTft();
      auto *t = displayManager.getCurrentThemeColors();
      int H = tft->height();
      // BTN1
      if (activeBtn == 1) {
        tft->fillRoundRect(8, H - 52, 106, 22, 6, t->accent_primary);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(t->background_dark, t->accent_primary);
        tft->drawString("BTN1: WiFi QR", 8 + 53, H - 41);
        tft->fillRoundRect(126, H - 52, 106, 22, 6, t->background_light);
        tft->setTextColor(t->text_primary, t->background_light);
        tft->drawString("BTN2: Continue", 126 + 53, H - 41);
      } else {
        tft->fillRoundRect(8, H - 52, 106, 22, 6, t->background_light);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(t->text_primary, t->background_light);
        tft->drawString("BTN1: WiFi QR", 8 + 53, H - 41);
        tft->fillRoundRect(126, H - 52, 106, 22, 6, t->accent_primary);
        tft->setTextColor(t->background_dark, t->accent_primary);
        tft->drawString("BTN2: Continue", 126 + 53, H - 41);
      }
    };

    unsigned long apScreenStart = millis();
    const unsigned long AP_SCREEN_TIMEOUT = 30000;
    bool inQrMode = false;
    int lastSecond = 30;
    unsigned long qrStartTime = 0;
    int lastQrSecond = 30;

    while (millis() - apScreenStart < AP_SCREEN_TIMEOUT) {
      esp_task_wdt_reset();

      int secondsLeft =
          (int)((AP_SCREEN_TIMEOUT - (millis() - apScreenStart)) / 1000) + 1;

      if (inQrMode) {
        // Manually update QR timer without triggering status bar
        unsigned long now = millis();
        int qrSecondsLeft = (int)((30000 - (now - qrStartTime)) / 1000);
        if (qrSecondsLeft >= 0 && qrSecondsLeft != lastQrSecond) {
          lastQrSecond = qrSecondsLeft;
          displayManager.updateQRTimer(qrSecondsLeft);
        }

        if (digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW) {
          while (digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW) {
            esp_task_wdt_reset();
            delay(10);
          }
          displayManager.hideQRCode();
          inQrMode = false;
          lastSecond = -1;
          drawApInfoScreen();
        }
        delay(50);
        continue;
      }

      if (secondsLeft != lastSecond) {
        lastSecond = secondsLeft;
        TFT_eSPI *tft = displayManager.getTft();
        tft->setTextDatum(MC_DATUM);
        tft->setTextSize(1);
        tft->setTextColor(secondsLeft <= 5 ? TFT_RED : TFT_YELLOW, TFT_BLACK);
        tft->fillRect(0, 116, tft->width(), 14, TFT_BLACK);
        tft->drawString("Auto: " + String(secondsLeft) + "s", tft->width() / 2,
                        122);
      }

      if (digitalRead(BUTTON_1) == LOW) {
        highlightButton(1);
        while (digitalRead(BUTTON_1) == LOW) {
          esp_task_wdt_reset();
          delay(10);
        }
        delay(30);
        esp_task_wdt_reset();
        String wifiQR =
            "WIFI:S:" + apName + ";T:WPA;P:" + apPassword + ";H:false;;";
        LOG_INFO("Main", "WiFi QR: " + wifiQR);
        displayManager.showQRCode(wifiQR, 30);
        qrStartTime = millis();
        lastQrSecond = 30;
        inQrMode = true;
        continue;
      }

      if (digitalRead(BUTTON_2) == LOW) {
        highlightButton(2);
        while (digitalRead(BUTTON_2) == LOW) {
          esp_task_wdt_reset();
          delay(10);
        }
        delay(30);
        esp_task_wdt_reset();
        break;
      }

      delay(50);
    }

    // Автозапуск веб-сервера в AP режиме
    LOG_INFO("Main", "Auto-starting Web Server in AP Mode...");
    webServerManager.start();

    if (displayManager.isQRCodeActive())
      displayManager.hideQRCode();
    displayManager.clearMessageArea(0, 0, displayManager.getTft()->width(), displayManager.getTft()->height());

    // ❗ ПРОПУСКАЕМ WiFi подключение и синхронизацию времени
    // TOTP коды будут показывать "TIME NOT SYNCED"
    timeSynced = false;

  } else if (selectedMode == StartupMode::OFFLINE_MODE) {
    // 🔌 OFFLINE MODE
    LOG_INFO("Main", "User chose Offline Mode. No WiFi, no AP, no web server.");

    // Check if web server should auto-start (overrides offline mode for one boot)
    bool autoStartWebServer = configManager.getWebServerAutoStart();
    if (autoStartWebServer) {
      LOG_INFO("Main", "Auto-start web server flag set — overriding offline mode for this boot.");
      configManager.setWebServerAutoStart(false);
      
      if (wifiManager.connectSilent()) {
        LOG_INFO("Main", "WiFi connected. Starting Web Server (offline override).");
        webServerManager.start();
        delay(500);
        showWebServerInfoPage();
      } else {
        LOG_ERROR("Main", "WiFi reconnection failed! Web server not started. Falling back to offline.");
        displayManager.init();
        displayManager.showMessage("ERROR:", 10, 20, true, 2);
        displayManager.showMessage("WiFi reconnect failed!", 10, 40, false, 2);
        delay(2000);
        displayManager.clearMessageArea(0, 0, displayManager.getTft()->width(), displayManager.getTft()->height());
        WiFi.mode(WIFI_OFF);
      }
      timeSynced = totpGenerator.isTimeSynced();
    } else {
      // Полное отключение WiFi
      WiFi.mode(WIFI_OFF);

    // Отображение информации
    displayManager.init();
    displayManager.showMessage("Offline Mode", 10, 20, false, 2);
    displayManager.showMessage("No WiFi Connection", 10, 40, false, 1);
    displayManager.showMessage("BLE & Passwords Work", 10, 55, false, 1);
    if (totpGenerator.isTimeSynced()) {
      displayManager.showMessage("TOTP: Synced via RTC", 10, 70, false, 1);
    } else {
      displayManager.showMessage("TOTP: NOT SYNCED", 10, 70, false, 1);
    }
    delay(3000);

    // Очистка экрана
    displayManager.clearMessageArea(0, 0, displayManager.getTft()->width(), displayManager.getTft()->height());

    // ❗ ПРОПУСКАЕМ: WiFi, веб-сервер, синхронизацию времени
    // Работают только: TOTP (несинхронизированный), пароли, BLE
    timeSynced = totpGenerator.isTimeSynced();
    } // end autoStartWebServer else

  } else {
    // 🌐 WIFI MODE (по умолчанию)
    LOG_INFO("Main",
             "User chose WiFi mode (or timeout). Connecting to WiFi...");
    displayManager.updateMessage("Connecting WiFi...", 10, 10, 2);

    if (!wifiManager.connect()) {
      LOG_WARNING("Main",
                  "No WiFi credentials found. Starting config portal...");

      // 🚫 Отключаем watchdog перед Config Portal (иначе async_tcp вызывает
      // timeout)
      esp_task_wdt_deinit();
      LOG_INFO("Main", "Watchdog disabled for Config Portal mode");

      wifiManager.startConfigPortal();
      webServerManager.startConfigServer();

      delay(500); // ждём стабилизации async_tcp

      esp_task_wdt_delete(NULL);
      TaskHandle_t asyncTcpTask = xTaskGetHandle("async_tcp");
      if (asyncTcpTask != NULL) {
        esp_task_wdt_delete(asyncTcpTask);
      }
      esp_task_wdt_deinit();

      wifiManager.startDns(); // только после отключения WDT

      configPortalActive = true;
      return; // передаём управление loop()
    }
    LOG_INFO("Main", "WiFi Connected! IP: " + wifiManager.getIP());

    // 🕗 Time Sync (только для WiFi Mode)
    // 🌍 Используем разные NTP сервера для повышения надежности
    const char *ntpServers[] = {
        "time.google.com",    // Google NTP (fast & reliable) - ПЕРВЫЙ
        "pool.ntp.org",       // Global NTP pool
        "time.cloudflare.com" // Cloudflare NTP (1.1.1.1)
    };

    LOG_INFO("Main", "Syncing time with multiple NTP servers...");
    for (int i = 0; i < 3; i++) {
      // 🔄 Обновляем только текст без полной перерисовки
      displayManager.updateMessage("Time Sync... (" + String(i + 1) + "/3)", 10,
                                   10, 2);

      // ✅ Каждая попытка использует СВОЙ NTP сервер
      LOG_INFO("Main",
               "NTP attempt " + String(i + 1) + ": " + String(ntpServers[i]));
      configTime(0, 0, ntpServers[i]);

      // Даем время на отправку и обработку NTP запроса
      delay(800); // Увеличено с 500ms для стабильности

      String savedTz = configManager.getTimezone();
      setenv("TZ", savedTz.c_str(), 1);
      tzset();
      if (getLocalTime(&timeinfo, 5000)) {
        timeSynced = true;
        LOG_INFO("Main", "Time Synced Successfully on attempt " +
                             String(i + 1) + " (" + String(ntpServers[i]) +
                             ")!");
        // Write to DS3231 immediately after NTP confirm, before any display delays
        if (rtcManager.getConfig().enabled && rtcManager.isAvailable()) {
          rtcManager.syncToRTC(0);  // 0 = use internal gettimeofday + second boundary wait
          LOG_INFO("Main", "DS3231 updated from NTP");
        }
        // 🔄 Обновляем только текст
        displayManager.updateMessage("Time Synced!", 10, 10, 2);
        delay(1000);
        break;
      }

      LOG_WARNING("Main", "NTP server " + String(ntpServers[i]) + " failed");

      // ⌨️ Задержка перед следующей попыткой (кроме последней)
      if (i < 2) {
        delay(1000); // 1 секунда между попытками
      }
    }

    if (!timeSynced) {
      // Try DS3231 as fallback if NTP failed
      if (rtcManager.getConfig().enabled && rtcManager.isAvailable() && rtcManager.syncFromRTC()) {
        timeSynced = true;
        LOG_INFO("Main", "Time recovered from DS3231 after NTP failure");
        displayManager.updateMessage("Time from RTC!", 10, 10, 2);
        delay(1000);
      }
    }

    if (!timeSynced) {
      // ⚠️ OFFLINE FALLBACK: Продолжаем работу без синхронизации времени
      // TOTP будет показывать "NOT SYNCED", но пароли и BLE работают нормально
      LOG_WARNING("Main", "All 3 NTP servers failed (time.google.com, "
                          "pool.ntp.org, time.cloudflare.com)");
      LOG_WARNING(
          "Main",
          "Continuing in offline mode. TOTP: NOT SYNCED, Passwords: OK");

      displayManager.init();
      displayManager.showMessage("WARNING:", 10, 20, false, 2);
      displayManager.showMessage("Time sync failed!", 10, 40, false, 2);
      displayManager.showMessage("TOTP: NOT SYNCED", 10, 60, false, 1);
      displayManager.showMessage("Passwords: OK", 10, 75, false, 1);
      displayManager.showMessage("Continuing...", 10, 95, false, 1);
      delay(3000);

      // Устанавливаем timeSynced = false для offline режима
      timeSynced = false;
    }

    // Отключаем WiFi для экономии батареи (независимо от статуса синхронизации)
    if (timeSynced) {
      LOG_INFO("Main",
               "Time synced successfully. Disconnecting WiFi to save power.");
    } else {
      LOG_INFO("Main", "Disconnecting WiFi to save power (time not synced).");
    }
    wifiManager.disconnect();

    // Check if web server should auto-start
    bool autoStartWebServer = configManager.getWebServerAutoStart();
    if (autoStartWebServer) {
      LOG_INFO("Main", "Auto-starting Web Server (flag was set)...");
      // Reset the flag immediately to prevent auto-start on subsequent boots
      configManager.setWebServerAutoStart(false);

      if (wifiManager.connectSilent()) {
        LOG_INFO("Main", "WiFi Reconnected. Starting Web Server.");
        webServerManager.start();
        delay(500); // Даём время async web server полностью инициализироваться
        showWebServerInfoPage();
      } else {
        LOG_ERROR("Main", "WiFi reconnection failed! Web server not started.");
        displayManager.init();
        displayManager.showMessage("ERROR:", 10, 20, true, 2);
        displayManager.showMessage("WiFi reconnect failed!", 10, 40, false, 2);
        delay(2000);
      }
    } else {
      LOG_INFO("Main", "Prompting for Web Server...");
      if (displayManager.promptWebServerSelection()) {
        LOG_INFO("Main",
                 "User chose to start Web Server. Reconnecting to WiFi...");
        if (wifiManager.connectSilent()) {
          LOG_INFO("Main", "WiFi Reconnected. Starting Web Server.");
          webServerManager.start();
          delay(
              500); // Даём время async web server полностью инициализироваться
          showWebServerInfoPage();
        } else {
          LOG_ERROR("Main",
                    "WiFi reconnection failed! Web server not started.");
          displayManager.init();
          displayManager.showMessage("ERROR:", 10, 20, true, 2);
          displayManager.showMessage("WiFi reconnect failed!", 10, 40, false,
                                     2);
          delay(2000);
        }
      } else {
        LOG_INFO("Main", "User chose not to start Web Server.");
        // ✅ Промптинг уже очистил экран перед return
      }
    }
  } // Конец WiFi Mode

  // ✅ displayManager.init() уже вызван - очищаем область сообщений перед
  // входом в основной цикл
  displayManager.clearMessageArea(0, 0, displayManager.getTft()->width(), 60);

  LOG_INFO("Main", "Main Loop Started");
  lastActivityTime = millis();

  LOG_INFO("Main", "Initializing Watchdog Timer...");
  if (esp_task_wdt_init(WDT_TIMEOUT, true) == ESP_OK) {
    if (esp_task_wdt_add(NULL) == ESP_OK) {
      LOG_INFO("Main", "Watchdog Timer initialized successfully");
    } else {
      LOG_ERROR("Main", "Failed to add task to Watchdog Timer");
    }
  } else {
    LOG_ERROR("Main", "Failed to initialize Watchdog Timer");
  }
}

void handleButtons() {
  esp_task_wdt_reset();
  static unsigned long button1PressStartTime = 0;
  static unsigned long button2PressStartTime = 0;
  static bool suppressNextSinglePress = false;
  bool buttonPressed = false;

  bool button1_is_pressed = (digitalRead(BUTTON_1) == LOW);
  bool button2_is_pressed = (digitalRead(BUTTON_2) == LOW);

  // --- Логика двойного нажатия (высший приоритет) ---
  if (button1_is_pressed && button2_is_pressed) {
    // Если зажаты обе кнопки, сбрасываем таймеры одиночных нажатий, чтобы
    // предотвратить конфликт
    button1PressStartTime = 0;
    button2PressStartTime = 0;

    // HOTP generation: both buttons held 1 second in TOTP mode
    if (currentMode == AppMode::TOTP) {
      auto keys = keyManager.getAllKeys();
      if (!keys.empty() && keys[currentKeyIndex].type == TOTPType::HOTP) {
        if (bothButtonsPressStartTime == 0) {
          bothButtonsPressStartTime = millis();
        }
        unsigned long holdTime = millis() - bothButtonsPressStartTime;

        if (holdTime < 2000) {
          int progress = map(holdTime, 0, 2000, 0, 100);
          displayManager.drawHOTPLoader(progress);
          esp_task_wdt_reset();
        }
        if (holdTime >= 2000 && !bleActionTriggered) {
          bleActionTriggered = true;
          hotpSavePending = true;
          hotpSaveIndex = currentKeyIndex;
          // Nothing else — return immediately, main loop handles the rest
          return;
        }
        return; // return only for HOTP keys
      }
      // For non-HOTP keys in TOTP mode — fall through, no double-press action
      return; // still prevent BLE activation in TOTP mode
    }

    // Действие по двойному нажатию валидно только в режиме паролей
    if (currentMode == AppMode::PASSWORD &&
        !passwordManager.getAllPasswords().empty()) {
      if (bothButtonsPressStartTime == 0) {
        bothButtonsPressStartTime = millis();
      }

      unsigned long holdTime = millis() - bothButtonsPressStartTime;

      if (holdTime >= 500) {    // Показываем лоадер через 500мс
        if (holdTime >= 2000) { // Если продержали 2с
          if (!bleActionTriggered) {
            bleActionTriggered = true;
            currentMode = AppMode::BLE_ADVERTISING;
            previousPasswordIndex = -1;
            LOG_INFO("Main",
                     "Both buttons held. Switching to BLE_ADVERTISING mode");
          }
        } else {
          // Рисуем лоадер для BLE
          int progress = map(holdTime - 500, 0, 1500, 0, 100);
          displayManager.drawBleInitLoader(progress);
        }
      }
    }
    // В любом случае выходим, чтобы не обрабатывать одиночные нажатия
    return;
  } else {
    // Если кнопки не зажаты вместе, сбрасываем таймер двойного нажатия
    if (bothButtonsPressStartTime > 0) {
      bothButtonsPressStartTime = 0;
      suppressNextSinglePress = true;
      bleActionTriggered = false;
      previousPasswordIndex = -1;

      // Don't call hideLoader in TOTP mode - it clears the screen and wipes the
      // code
      if (currentMode != AppMode::TOTP) {
        displayManager.hideLoader();
      } else {
        displayManager.eraseLoaderArea();
        previousKeyIndex = -1; // force TOTP display redraw
      }

      // Сбрасываем одиночные таймеры чтобы предотвратить ложные нажатия после
      // двойного
      button1PressStartTime = 0;
      button2PressStartTime = 0;
    }
  }

  // Suppress single button processing until both buttons are fully released
  if (suppressNextSinglePress) {
    if (!button1_is_pressed && !button2_is_pressed) {
      suppressNextSinglePress = false;
    }
    return;
  }

  // --- Логика одиночных нажатий (выполняется, только если не зажаты обе
  // кнопки) ---

  // --- Кнопка 1 (GPIO 35) ---
  if (button1_is_pressed) {
    if (button1PressStartTime == 0) {
      button1PressStartTime = millis();
    } else if (millis() - button1PressStartTime > powerOffHoldTime) {
      // Длительное нажатие: переключить режим
      LOG_INFO("Main", "Button 1 LONG PRESS: Switching modes...");
      if (currentMode == AppMode::BLE_ADVERTISING ||
          currentMode == AppMode::BLE_PIN_ENTRY ||
          currentMode == AppMode::BLE_CONFIRM_SEND) {
        bleKeyboardManager.end();
        bleActionTriggered = false;
      }
      currentMode =
          (currentMode == AppMode::TOTP) ? AppMode::PASSWORD : AppMode::TOTP;
      LOG_INFO("Main", currentMode == AppMode::TOTP
                           ? "Switched to TOTP mode"
                           : "Switched to PASSWORD mode");
      button1PressStartTime = 0;
      buttonPressed = true;
      previousKeyIndex = -1;
      previousPasswordIndex = -1;
      displayManager.hideLoader();
    } else {
      unsigned long holdTime = millis() - button1PressStartTime;
      if (holdTime >= 1000 && holdTime < powerOffHoldTime) {
        int progress = map(holdTime - 1000, 0, 4000, 0, 100);
        String loaderText =
            (currentMode == AppMode::TOTP) ? "Passwords..." : "TOTP...";
        displayManager.drawGenericLoader(progress, loaderText);
      }
    }
  } else {
    if (button1PressStartTime > 0) {
      displayManager.hideLoader();
      if (millis() - button1PressStartTime < powerOffHoldTime) {
        LOG_DEBUG("Main", "Button 1 press: Previous item");
        if (currentMode == AppMode::TOTP) {
          auto keys = keyManager.getAllKeys();
          if (!keys.empty()) {
            currentKeyIndex =
                (currentKeyIndex == 0) ? keys.size() - 1 : currentKeyIndex - 1;
            displayManager.setKeySwitched(true); // <-- ADDED
            buttonPressed = true;
          }
        } else if (currentMode == AppMode::PASSWORD) {
          auto passwords = passwordManager.getAllPasswords();
          if (!passwords.empty()) {
            currentPasswordIndex = (currentPasswordIndex == 0)
                                       ? passwords.size() - 1
                                       : currentPasswordIndex - 1;
            buttonPressed = true;
          }
        }
      }
      button1PressStartTime = 0;
    }
  }

  // --- Кнопка 2 (GPIO 0) ---
  if (button2_is_pressed) {
    if (button2PressStartTime == 0) {
      button2PressStartTime = millis();
    } else if (millis() - button2PressStartTime > powerOffHoldTime) {
      LOG_INFO("Main", "Button 2 LONG PRESS: Shutting down...");
      displayManager.init();
      displayManager.showMessage("Shutting down...", 10, 30, false, 2);
      delay(1000);
      displayManager.turnOff();
      secureShutdown();
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
      esp_deep_sleep_start();
    } else {
      unsigned long holdTime = millis() - button2PressStartTime;
      if (holdTime >= 1000 && holdTime < powerOffHoldTime) {
        int progress = map(holdTime - 1000, 0, 4000, 0, 100);
        displayManager.drawGenericLoader(progress, "Shutting down...");
      }
    }
  } else {
    if (button2PressStartTime > 0) {
      displayManager.hideLoader();
      if (millis() - button2PressStartTime < powerOffHoldTime) {
        LOG_DEBUG("Main", "Button 2 press: Next item");
        if (currentMode == AppMode::TOTP) {
          auto keys = keyManager.getAllKeys();
          if (!keys.empty()) {
            currentKeyIndex = (currentKeyIndex + 1) % keys.size();
            displayManager.setKeySwitched(true); // <-- ADDED
            buttonPressed = true;
          }
        } else if (currentMode == AppMode::PASSWORD) {
          auto passwords = passwordManager.getAllPasswords();
          if (!passwords.empty()) {
            currentPasswordIndex =
                (currentPasswordIndex + 1) % passwords.size();
            buttonPressed = true;
          }
        }
      }
      button2PressStartTime = 0;
    }
  }

  if (buttonPressed) {
    lastActivityTime = millis();
    displayManager.hideLoader();
    if (!isScreenOn) {
      LOG_DEBUG("Main", "Button press woke up screen");
      displayManager.turnOn();
      isScreenOn = true;
    }
    previousKeyIndex = -1;
    previousPasswordIndex = -1;
  }
}

// Функция для проверки нажатия кнопок и включения экрана
void checkScreenWakeup() {
  static bool button1PreviousState = HIGH;
  static bool button2PreviousState = HIGH;

  bool button1Current = digitalRead(BUTTON_1);
  bool button2Current = digitalRead(BUTTON_2);

  // Проверяем нажатие любой кнопки (переход от HIGH к LOW)
  if ((button1PreviousState == HIGH && button1Current == LOW) ||
      (button2PreviousState == HIGH && button2Current == LOW)) {

    lastActivityTime = millis();

    // If QR code active - dismiss it on any button press
    if (displayManager.isQRCodeActive()) {
      LOG_INFO("Main", "Button pressed - hiding QR code");
      displayManager.hideQRCode();
      return;
    }

    if (!isScreenOn) {
      LOG_DEBUG("Main", "Button press woke up screen in BLE mode");
      displayManager.turnOn();
      isScreenOn = true;
    }
  }

  button1PreviousState = button1Current;
  button2PreviousState = button2Current;
}

void loop() {
  if (configPortalActive) {
    wifiManager.processDnsRequests();
    delay(1);
    return;
  }

  // Сброс Watchdog Timer в начале каждого цикла
  if (esp_task_wdt_reset() != ESP_OK) {
    LOG_ERROR("Main", "Failed to reset Watchdog Timer");
  }
  displayManager.update(); // Обновляем анимации в любом режиме

  // Всегда проверяем включение экрана от кнопок
  checkScreenWakeup();

  // Pending theme change from web server (deferred to avoid watchdog in async_tcp)
  if (pendingThemeChange) {
    pendingThemeChange = false;
    LOG_INFO("Main", "Applying pending theme change from web server");
    displayManager.setTheme(pendingTheme);
  }

  // НОВАЯ ЛОГИКА: Проверка флага отключения PIN (устанавливается веб-сервером)
  if (shouldPromptPinDisable) {
    shouldPromptPinDisable = false; // Сбрасываем сразу

    LOG_INFO("Main", "PIN disable request detected - prompting on device");

    // Включаем экран если он выключен
    if (!isScreenOn) {
      displayManager.turnOn();
      isScreenOn = true;
    }

    // Показываем сообщение
    displayManager.init();
    displayManager.showMessage("Web Request:", 10, 20, false, 2);
    displayManager.showMessage("Disable PIN?", 10, 40, false, 2);
    displayManager.showMessage("Enter PIN to confirm", 10, 70, false, 1);
    delay(2000);

    // Запрашиваем PIN на устройстве
    String enteredPin = pinManager.requestPinInput("Confirm Disable PIN");

    if (enteredPin.length() > 0) {
      // PIN введен, пытаемся отключить защиту
      displayManager.init();
      displayManager.showMessage("Disabling PIN...", 10, 30, false, 2);

      if (CryptoManager::getInstance().disablePinEncryption(enteredPin)) {
        LOG_INFO("Main", "PIN protection disabled via device confirmation");

        displayManager.init();
        displayManager.showMessage("PIN Disabled!", 10, 30, false, 2);
        displayManager.showMessage("Rebooting...", 10, 60, false, 2);
        delay(2000);

        // Перезагружаемся
        ESP.restart();
      } else {
        LOG_ERROR("Main", "Failed to disable PIN - wrong PIN");

        displayManager.init();
        displayManager.showMessage("Wrong PIN!", 10, 30, true, 2);
        displayManager.showMessage("Try again via web", 10, 60, false, 1);
        delay(3000);
      }
    } else {
      LOG_INFO("Main", "PIN disable cancelled by user");

      displayManager.init();
      displayManager.showMessage("Cancelled", 10, 30, false, 2);
      delay(1000);
    }

    displayManager.init(); // Возвращаемся к нормальному экрану
  }

  // Проверяем таймаут API веб-сервера и самого сервера
  if (webServerManager.isRunning()) {
    WebAdminManager::getInstance().checkApiTimeout();
    webServerManager.update();

    // Process DNS for Admin AP mode
    if (selectedMode == StartupMode::AP_MODE) {
      adminDnsServer.processNextRequest();
    }

#ifdef SECURE_LAYER_ENABLED
    // ❌ DISABLED: Cleanup causes race condition without mutex
    // secureLayerManager.update();
    // ✅ ENABLED: Traffic Obfuscation for decoy traffic generation
    trafficObfuscationManager.update();
    // ✅ ENABLED: URL Obfuscation automatic rotation (daily)
    URLObfuscationManager::getInstance().update();
#endif
  }

  // Handle buttons based on the current mode for BLE states
  if (currentMode != AppMode::BLE_ADVERTISING &&
      currentMode != AppMode::BLE_PIN_ENTRY &&
      currentMode != AppMode::BLE_CONFIRM_SEND) {
    handleButtons();
  }

  // HOTP generation state machine — all heavy work done here in main loop
  if (hotpSavePending) {
    // Wait until both buttons released
    if (digitalRead(BUTTON_1) == HIGH && digitalRead(BUTTON_2) == HIGH) {
      delay(50); // stabilization
      esp_task_wdt_reset();

      // Now do all heavy work from stable main loop context
      keyManager.getKeyRef(hotpSaveIndex).counter++;
      auto updatedKeys = keyManager.getAllKeys();
      String newCode = totpGenerator.generateCode(updatedKeys[hotpSaveIndex]);
      displayManager.updateTOTPCode(newCode, -1);
      displayManager.eraseLoaderArea();
      esp_task_wdt_reset();
      keyManager.saveKeys();
      esp_task_wdt_reset();

      hotpSavePending = false;
      hotpSaveIndex = -1;
      LOG_INFO("Main", "HOTP code generated and saved");
    }
  }

  // Проверяем таймаут экрана ТОЛЬКО если веб-сервер НЕ активен
  // Когда веб-сервер работает, таймаут экрана полностью игнорируется для
  // предотвращения замедлений
  uint16_t screenTimeoutSeconds = configManager.getDisplayTimeout();

  // ВАЖНО: Когда веб-сервер активен, сбрасываем lastActivityTime чтобы при его
  // выключении был нормальный отсчет таймаута, а не моментальный переход в
  // sleep
  if (webServerManager.isRunning()) {
    lastActivityTime = millis();
  }

  // Independent auto lock timer — only when screen timeout is Never (0)
  // When screen timeout > 0, auto lock runs inside pseudo-sleep polling loop
  uint32_t autoLockSecondsMain = configManager.getAutoLockTimeout();
  if (!webServerManager.isRunning() && screenTimeoutSeconds == 0 &&
      !bleKeyboardManager.isConnected() && autoLockSecondsMain > 0 &&
      (millis() - lastActivityTime > (autoLockSecondsMain * 1000UL))) {
    LOG_INFO("Main", "Auto lock timeout reached (screen=Never mode). Entering deep sleep.");
    displayManager.turnOff();
    secureShutdown();
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
  }

  if (!webServerManager.isRunning() && screenTimeoutSeconds > 0 && isScreenOn &&
      !bleKeyboardManager.isConnected() &&
      (millis() - lastActivityTime > (screenTimeoutSeconds * 1000))) {

    // Веб-сервер не активен - можно засыпать
    LOG_INFO(
        "Main",
        "Screen timeout reached. Web server inactive. Entering pseudo-sleep.");

    // Отключаем BLE для безопасности, если он активен
    if (currentMode == AppMode::BLE_ADVERTISING ||
        currentMode == AppMode::BLE_PIN_ENTRY ||
        currentMode == AppMode::BLE_CONFIRM_SEND) {
      LOG_INFO("Main", "Disabling BLE due to screen timeout for security");
      bleKeyboardManager.end();
      currentMode = AppMode::PASSWORD;
      bleActionTriggered = false;
    }

    // 1. Выключаем дисплей перед сном
    displayManager.turnOff();
    isScreenOn = false;

    // 2. Pseudo-sleep: lower CPU frequency, poll buttons
    // esp_light_sleep_start() causes POWER_ON reset on battery due to
    // high internal resistance — voltage drops below RTC threshold on CPU wake spike.
    // Hardware-incompatible without bulk capacitor on power rail.
    uint32_t autoLockSeconds = configManager.getAutoLockTimeout();
    LOG_INFO("Main", "Entering pseudo-sleep (40MHz CPU, display off, TFT sleep). AutoLock=" + String(autoLockSeconds) + "s");
    
    Serial.flush();
    setCpuFrequencyMhz(40);
    Serial.updateBaudRate(115200);
    
    unsigned long pseudoSleepStart = millis();
    
    // Poll both buttons — wake on any press, or auto lock on timeout
    while (digitalRead(BUTTON_1) == HIGH && digitalRead(BUTTON_2) == HIGH) {
      esp_task_wdt_reset();
      
      if (autoLockSeconds > 0 &&
          (millis() - pseudoSleepStart > (autoLockSeconds * 1000UL))) {
        setCpuFrequencyMhz(240);
        Serial.updateBaudRate(115200);
        LOG_INFO("Main", "Auto lock timeout reached. Entering deep sleep.");
        displayManager.turnOff();
        secureShutdown();
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
        esp_deep_sleep_start();
        // Never reaches here
      }
      
      delay(50);
    }
    
    // Wait for button release before resuming
    while (digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW) {
      esp_task_wdt_reset();
      delay(10);
    }
    
    // Exit pseudo-sleep: restore full CPU speed
    setCpuFrequencyMhz(240);
    Serial.updateBaudRate(115200);
    
    LOG_INFO("Main", "Woke up from pseudo-sleep.");
    lastActivityTime = millis();
    if (!isScreenOn) {
      displayManager.turnOn(); // sends TFT_SLPOUT + 120ms delay + backlight
      isScreenOn = true;
    }
    // Re-sync system clock from DS3231 after wake
    if (rtcManager.getConfig().enabled && rtcManager.isAvailable()) {
      delay(5);
      rtcManager.syncFromRTC();
    }
  }

  if (isScreenOn) {
    // Пропускаем обновления если активен лоадер или QR код
    if (!displayManager.isLoaderActive() && !displayManager.isQRCodeActive()) {
      // Обновляем статус батареи по таймеру (общий для всех режимов)
      if (millis() - lastBatteryCheckTime > batteryCheckInterval) {
        lastBatteryCheckTime = millis();
        isCharging = (batteryManager.getVoltage() > 4.15);
        if (!isCharging) {
          _lastRealBatteryPercentage = batteryManager.getPercentage();
        }
        if (chargeDisplayPct == -1) {
          _lastRealBatteryPercentage = batteryManager.getPercentage();
          chargeDisplayPct = _lastRealBatteryPercentage;
        }
        chargeDisplayPct = isCharging ? 0 : _lastRealBatteryPercentage;
        displayManager.updateBatteryStatus(chargeDisplayPct, isCharging);
        displayManager.updateClockStatus();
      }

      // Мониторинг критического состояния памяти
      static unsigned long lastCriticalMemoryCheck = 0;
      if (millis() - lastCriticalMemoryCheck >
          30000) { // Проверяем каждые 30 секунд
        lastCriticalMemoryCheck = millis();
        uint32_t freeHeap = ESP.getFreeHeap();

        // Только критические предупреждения для production
        if (freeHeap < 15000) { // Меньше 15KB - критично!
          LOG_CRITICAL("Memory",
                       "CRITICAL LOW MEMORY! Device may become unstable");

          // Принудительная очистка кэшей при критической нехватке памяти
          if (freeHeap < 10000) {
            ESP.restart(); // Аварийная перезагрузка при < 10KB
          }
        }
      }
    }

    switch (currentMode) {
    case AppMode::TOTP: {
      auto keys = keyManager.getAllKeys();
      if (!keys.empty()) {
        if (currentKeyIndex != previousKeyIndex) {
          displayManager.drawLayout(
              keys[currentKeyIndex].name, chargeDisplayPct,
              isCharging, webServerManager.isRunning());
          previousKeyIndex = currentKeyIndex;
        }

        if (!displayManager.isLoaderActive() &&
            !displayManager.isQRCodeActive() &&
            millis() - lastTotpUpdateTime > totpUpdateInterval) {
          lastTotpUpdateTime = millis();

          // 1. HOTP keys work without time synchronization
          if (keys[currentKeyIndex].type == TOTPType::HOTP) {
            String code = totpGenerator.generateCode(keys[currentKeyIndex]);
            displayManager.updateTOTPCode(code, -1, 30);
          }
          // 2. TOTP keys require time sync
          else if (!totpGenerator.isTimeSynced()) {
            displayManager.updateTOTPCode("NOT SYNCED", 0);

            if (WiFi.getMode() == WIFI_STA && WiFi.isConnected()) {
              static unsigned long lastWarningTime = 0;
              static bool warningsShown = false;

              if (currentKeyIndex != previousKeyIndex ||
                  millis() - lastWarningTime > 5000 || !warningsShown) {

                lastWarningTime = millis();
                warningsShown = true;

                TFT_eSPI *tft = displayManager.getTft();
                int warnY = tft->height() - 60;
                tft->fillRect(
                    0, warnY, tft->width(), 60,
                    displayManager.getCurrentThemeColors()->background_dark);

                tft->setTextDatum(MC_DATUM);
                tft->setTextColor(
                    displayManager.getCurrentThemeColors()->text_secondary,
                    displayManager.getCurrentThemeColors()->background_dark);
                tft->setTextSize(1);
                tft->drawString("⚠️ Connect to network", tft->width() / 2, warnY + 5);
                tft->drawString("for time sync", tft->width() / 2, warnY + 20);
                tft->drawString("or switch to passwords", tft->width() / 2, warnY + 30);
                tft->drawString("(Hold BTN1)", tft->width() / 2, warnY + 40);
              }
            }
          } else {
            // 3. TOTP: Time is synced
            String code = totpGenerator.generateCode(keys[currentKeyIndex]);
            int timeLeft =
                totpGenerator.getTimeRemaining(keys[currentKeyIndex].period);
            displayManager.updateTOTPCode(code, timeLeft,
                                          keys[currentKeyIndex].period);
          }
        }
      } else {
        displayManager.drawNoItemsPage("keys");
      }
      break;
    }
    case AppMode::PASSWORD: {
      auto passwords = passwordManager.getAllPasswords();
      if (!passwords.empty()) {
        if (currentPasswordIndex != previousPasswordIndex) {
          displayManager.drawPasswordLayout(
              passwords[currentPasswordIndex].name,
              passwords[currentPasswordIndex].password,
              chargeDisplayPct,
              isCharging, webServerManager.isRunning());
          previousPasswordIndex = currentPasswordIndex;
        }
      } else {
        displayManager.drawNoItemsPage("passwords");
      }
      break;
    }
    case AppMode::BLE_ADVERTISING: {
      static bool bleInitialized = false;
      static bool devicePinChecked = false;
      static unsigned long bleStartTime = 0;

      if (!bleInitialized) {
        LOG_INFO("Main", "Entering BLE_ADVERTISING setup");

        // BLE режим не требует Device PIN - только BLE PIN проверяется позже
        // при передаче
        LOG_INFO("Main", "BLE mode activated. Device PIN is NOT required for "
                         "BLE advertising.");
        devicePinChecked = true;

        if (devicePinChecked) {
          if (webServerManager.isRunning()) {
            webServerManager.stop();
            LOG_INFO("Main", "Web server stopped");
          }
          wifiManager.disconnect();

          bool bleStarted = bleKeyboardManager.begin();
          if (bleStarted) {
            String pinMsg = "PIN: " + String(bleKeyboardManager.getStaticPIN());
            displayManager.drawBleAdvertisingPage(
                bleKeyboardManager.getDeviceName(), pinMsg, 0);
            bleInitialized = true;
            devicePinChecked = false; // Reset for next time
          } else {
            LOG_ERROR("Main", "Failed to start secure BLE");
            currentMode = AppMode::PASSWORD;
            bleActionTriggered = false;
            devicePinChecked = false;
          }
          bleStartTime = millis(); // Запоминаем время начала BLE режима
          LOG_INFO("Main", "BLE Keyboard started. Waiting for connection...");
        }
      }

      if (bleKeyboardManager.isConnected()) {
        if (bleKeyboardManager.isSecure()) {
          LOG_INFO("Main", "BLE secure connection established");
          currentMode = AppMode::BLE_PIN_ENTRY;
          bleInitialized = false; // Reset for next time
          bleStartTime = 0;
        } else {
          // Защита от спама - логировать только раз в секунду
          static unsigned long lastAuthLog = 0;
          static bool pinPageDrawn = false;

          if (millis() - lastAuthLog > 1000) {
            LOG_INFO(
                "Main",
                "BLE connected but not secure - waiting for authentication");
            lastAuthLog = millis();
          }

          if (!pinPageDrawn) {
            String pinMsg =
                "Enter PIN: " + String(bleKeyboardManager.getStaticPIN());
            displayManager.drawBleAdvertisingPage(
                bleKeyboardManager.getDeviceName(), pinMsg, 0);
            pinPageDrawn = true;
          }

          // Таймаут аутентификации - 30 секунд
          if (millis() - bleStartTime > 30000) {
            LOG_WARNING("Main", "Authentication timeout - disconnecting");
            bleKeyboardManager.end();
            currentMode = AppMode::PASSWORD;
            bleActionTriggered = false;
            bleInitialized = false;
            pinPageDrawn = false;
          }
        }
      }

      // Handle back button press
      if (digitalRead(BUTTON_1) == LOW) {
        delay(200); // Debounce
        LOG_INFO("Main", "Back button pressed - exiting BLE mode");
        bleKeyboardManager.end();
        currentMode = AppMode::PASSWORD;
        bleActionTriggered = false;
        bleInitialized = false;
        bleStartTime = 0;
      }
    } break;

    case AppMode::BLE_PIN_ENTRY: {
      LOG_INFO("Main", "BLE secure connection established. Checking Device BLE "
                       "PIN requirements...");

      bool pinOk = true;

      // Проверяем включен ли Device BLE PIN
      if (CryptoManager::getInstance().isDeviceBlePinEnabled()) {
        LOG_INFO("Main", "Device BLE PIN protection enabled, requesting PIN "
                         "for transmission...");
        pinOk = pinManager.requestDeviceBlePinForTransmission();
      } else {
        LOG_INFO("Main", "Device BLE PIN protection disabled. Proceeding to "
                         "send confirmation");
      }

      if (pinOk) {
        LOG_INFO("Main", "BLE access granted. Proceeding to send confirmation");
        currentMode = AppMode::BLE_CONFIRM_SEND;
      } else {
        LOG_WARNING("Main",
                    "Device BLE PIN incorrect or cancelled. Returning to list");
        bleKeyboardManager.end();
        currentMode = AppMode::PASSWORD;
        bleActionTriggered = false;
      }
    } break;

    case AppMode::BLE_CONFIRM_SEND: {
      static bool confirmPageDrawn = false;

      auto passwords = passwordManager.getAllPasswords();
      if (passwords.empty() || currentPasswordIndex >= passwords.size()) {
        // Safety check
        currentMode = AppMode::PASSWORD;
        bleKeyboardManager.end();
        bleActionTriggered = false;
        confirmPageDrawn = false;
        break;
      }

      // Рисуем страницу только один раз или при принудительной перерисовке
      if (!confirmPageDrawn || previousPasswordIndex == -1) {
        String passwordName = passwords[currentPasswordIndex].name;
        String password = passwords[currentPasswordIndex].password;
        String deviceName = bleKeyboardManager.getDeviceName();
        displayManager.drawBleConfirmPage(passwordName, password, deviceName);
        confirmPageDrawn = true;
        previousPasswordIndex = currentPasswordIndex;
      }

      // Wait for button press
      if (digitalRead(BUTTON_1) == LOW) { // Back button
        delay(200);
        LOG_INFO("Main", "Back button pressed. Cancelling send");
        currentMode = AppMode::PASSWORD;
        bleKeyboardManager.end();
        bleActionTriggered = false;
        confirmPageDrawn = false;
      } else if (digitalRead(BUTTON_2) == LOW) { // Send button
        delay(200);
        LOG_INFO("Main", "Send button pressed. Sending data");

        displayManager.drawBleSendingPage();
        String password = passwords[currentPasswordIndex].password;
        bleKeyboardManager.sendPassword(password.c_str());
        delay(500); // Give time for the UI and BLE

        displayManager.drawBleResultPage(true); // Show success
        delay(1500);

        // Возвращаемся к странице подтверждения для повторной отправки
        LOG_INFO("Main",
                 "Password sent successfully. Returning to confirmation page");
        previousPasswordIndex = -1; // Force redraw of confirm page
      }
    } break;
    }
  }

  // Check for scheduled restart
  if (shouldRestart) {
    LOG_INFO("Main", "Device restart requested. Restarting in 1 second...");
    delay(1000);
    ESP.restart();
  }
}
