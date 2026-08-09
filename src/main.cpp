#include "LittleFS.h"
#include "PasswordManager.h"
#include "app_modes.h" // Используем новый общий заголовок
#include "battery_manager.h"
#include "ble_keyboard_manager.h"
#ifdef BOARD_HAS_USB_HID
#include "usb_hid_manager.h"
UsbHidManager usbHidManager;
#endif
#include "board_config.h"
#include "button_helpers.h"
#include "config.h"
#include "config_manager.h"
#include "crypto_manager.h"
#include "secure_utils.h"
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
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
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>
#include <esp_partition.h>

#ifdef SECURE_LAYER_ENABLED
#include "secure_layer_manager.h"
#include "traffic_obfuscation_manager.h"
#include "url_obfuscation_manager.h"
#endif

// Global flag for device restart
bool shouldRestart = false;

// Global flag for PIN disable request (set by web server)
bool shouldPromptPinDisable = false;
bool shouldPromptRemoveHiddenSpace = false;

// Activity timestamp for restricted import/export mode (set by async handlers)
unsigned long lastRestrictedModeActivity = 0;

// Set by the "close session" handler in restricted mode to allow early,
// intentional exit instead of waiting for a timeout
bool importExportCompleted = false;

// Pending theme change (set by web server, applied in main loop to avoid watchdog)
bool pendingThemeChange = false;
Theme pendingTheme = Theme::LIGHT;

// Global display rotation (loaded once at startup for button helpers)
uint8_t g_displayRotation = 1;

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

BatteryManager batteryManager(BATTERY_ADC_PIN, BATTERY_ADC_CH, BATTERY_POWER_PIN);
BleKeyboardManager bleKeyboardManager(DEFAULT_BLE_DEVICE_NAME, "Lord", 100);

#ifdef BOARD_HAS_USB_HID
bool defaultHidIsBle = true;
#endif

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
void performDuressWipe() {
    LOG_CRITICAL("Main", "DURESS WIPE: Initiating");
    CryptoManager::getInstance().wipeDeviceKey();
    keyManager.wipeSecrets();
    passwordManager.wipePasswords();
    secureLayerManager.wipeAllSessions();
    webServerManager.clearSession();
    const char* files[] = {
        "/keys.json.enc", "/passwords.json.enc", "/wifi_config.json",
        "/wifi_config.json.enc", "/splash_config.json", "/pin_config.json",
        "/config.json", "/device.key", "/ble_config.json", "/.webadmin",
        "/mdns_config.json", "/.login_state", "/rtc_config.json",
        "/ble_pin.json.enc", "/device_ble_pin.json.enc", "/session.json.enc",
        "/duress_pin.hash", "/boot_counter.txt", "/.pin_attempts", 
        "/theme_pref.json", "/startup_pref.json", "/hid_mode.json", "/boot_mode.json", nullptr
    };
    for (int i = 0; files[i]; i++) LittleFS.remove(files[i]);
    fs::File root = LittleFS.open("/", "r");
    if (root) {
        fs::File file = root.openNextFile();
        while (file) {
            String name = String("/") + file.name(); file.close();
            if (name.startsWith("/url_mappings_")) LittleFS.remove(name);
            file = root.openNextFile();
        }
        root.close();
    }
    // 3. Unmount and erase entire LittleFS partition (forensic-grade wipe)
    // Partition label "spiffs" is the LittleFS data partition (~3.9 MB)
    LittleFS.end();
    const esp_partition_t* lfs_part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
        "spiffs");
    if (lfs_part) {
        esp_partition_erase_range(lfs_part, 0, lfs_part->size);
        // Explicit format — LittleFS.begin(format_if_failed) does not handle
        // LFS_ERR_CORRUPT (-84) on erased partitions in all esp_littlefs versions
        LittleFS.format();
    }

    // 4. Erase NVS partition (BLE bonding keys)
    nvs_flash_erase_partition("nvs");
    nvs_flash_init_partition("nvs");
    LOG_CRITICAL("Main", "DURESS WIPE: Complete. Restarting.");
    delay(500);
    ESP.restart();
}

void panicShutdown() {
  CryptoManager::getInstance().wipeDeviceKey();
  keyManager.wipeSecrets();
  passwordManager.wipePasswords();
}

// --- Password Wildcard: on-device random generation + session cache ---
// The generated value lives only in RAM for the duration of a single
// HID "unit session" (from entering HID transmission mode for this
// entry until a new entry/session starts). It is never persisted to
// flash and never appears in any API response.
static String generateWildcardPassword(int length) {
  static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?~";
  const size_t charsetLen = sizeof(charset) - 1; // exclude null terminator
  const uint8_t maxValid = (256 / charsetLen) * charsetLen; // avoid modulo bias
  String result = "";
  if (length < 1) length = 1;
  if (length > 64) length = 64;
  result.reserve(length);
  uint8_t randomByte;
  for (int i = 0; i < length; i++) {
    do {
      CryptoManager::getInstance().secureRandom(&randomByte, 1);
    } while (randomByte >= maxValid);
    result += charset[randomByte % charsetLen];
  }
  return result;
}

static String _wildcardSessionValue = "";
static int _wildcardSessionOwnerIndex = -1;

// Returns the cached value for this entry's current session, generating
// a fresh one only if this is a different entry than last time (i.e. a
// new session). Repeated calls for the same index return the SAME
// value — required so password+confirm-password fields match.
static String getWildcardSessionPassword(int index, int len) {
  if (_wildcardSessionOwnerIndex != index) {
    // Zero the previous cached value in place before it is discarded,
    // so the old heap buffer is never freed while still containing
    // a password (String::operator= frees the old buffer but does
    // not memset it).
    secureWipeString(_wildcardSessionValue);
    _wildcardSessionValue = generateWildcardPassword(len);
    _wildcardSessionOwnerIndex = index;
  }
  return _wildcardSessionValue;
}

// Wipes the cached session value from RAM. Called on secure shutdown
// and whenever a new HID session is about to start (forcing the next
// getWildcardSessionPassword() call to generate fresh).
static void wipeWildcardSession() {
  secureWipeString(_wildcardSessionValue);
  _wildcardSessionOwnerIndex = -1;
}

void secureShutdown() {
  LOG_INFO("Main", "Secure shutdown: wiping sensitive data...");
  wipeWildcardSession();
  CryptoManager::getInstance().wipeDeviceKey();
  keyManager.wipeSecrets();
  passwordManager.wipePasswords();
#ifdef SECURE_LAYER_ENABLED
  secureLayerManager.wipeAllSessions();
#endif
  delay(50);
  LOG_INFO("Main", "Secure shutdown: entering deep sleep");
}

void secureRestart() {
  LOG_INFO("Main", "Secure restart: wiping sensitive data...");
  wipeWildcardSession();
  CryptoManager::getInstance().wipeDeviceKey();
  keyManager.wipeSecrets();
  passwordManager.wipePasswords();
#ifdef SECURE_LAYER_ENABLED
  secureLayerManager.wipeAllSessions();
#endif
  delay(50);
  LOG_INFO("Main", "Secure restart: restarting device");
  ESP.restart();
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
  const int maxAttempts = 5;
  if (count < 0 || count > maxAttempts) {
    LOG_ERROR("Main", "Invalid PIN attempts value: " + String(count) + " — locking device");
    return maxAttempts;
  }
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

// ═══════════════════════════════════════════════════════════════════════════
// HIDDEN SPACE SETUP FLOW
// ═══════════════════════════════════════════════════════════════════════════

void runHiddenSpaceSetupFlow() {
  LOG_INFO("Main", "Starting secondary slot setup flow");

  // Hidden Space requires the startup PIN to be enabled — its dual-slot
  // format is built on top of an existing PIN-encrypted device key file.
  // Creating it on an unencrypted (plaintext) key file corrupts the file.
  if (!CryptoManager::getInstance().isDeviceKeyEncrypted()) {
    LOG_WARNING("Main", "Hidden Space setup blocked: Startup PIN is not enabled");
    displayManager.init();
    displayManager.showMessage("Cannot Create", 10, 20, true, 2);
    displayManager.showMessage("Hidden Space", 10, 45, true, 2);
    displayManager.showMessage("Enable Startup PIN first", 10, 70, false, 1);
    delay(3000);
    return;
  }
  
  // Load PIN length preference before any PIN entry
  // pinManager.begin() is not called yet, but loadPinConfig() alone
  // is sufficient to read /.sys_ui_prefs
  pinManager.loadPinConfig();
  int pinLen = pinManager.getPinLength();
  LOG_INFO("Main", ("Secondary slot setup: using PIN length " + String(pinLen) + " from primary context").c_str());
  
  displayManager.init();
  displayManager.showMessage("Hidden Space Setup", 10, 20, true, 2);
  delay(2000);
  
  // Step 1: Authenticate with Space A PIN first
  displayManager.init();
  displayManager.showMessage("Hidden Space Setup", 10, 20, true, 2);
  displayManager.showMessage("Enter main PIN", 10, 50, false, 1);
  displayManager.showMessage("to confirm", 10, 65, false, 1);
  delay(1500);
  
  String spaceAPin = pinManager.requestPinInput("Main PIN", false);
  
  if (spaceAPin.isEmpty()) {
    LOG_WARNING("Main", "Space A PIN entry cancelled");
    displayManager.init();
    displayManager.showMessage("Setup Cancelled", 10, 30, true, 2);
    delay(2000);
    secureWipeString(spaceAPin);
    secureRestart();
    return;
  }
  
  if (!CryptoManager::getInstance().unlockDeviceKeyWithPin(spaceAPin)) {
    LOG_ERROR("Main", "Primary context unlock failed in secondary slot setup");
    displayManager.init();
    displayManager.showMessage("Wrong PIN", 10, 30, true, 2);
    delay(2000);
    secureWipeString(spaceAPin);
    secureRestart();
    return;
  }
  
  LOG_INFO("Main", "Space A unlocked, active space: A");
  // Now _activeSpace == ActiveSpace::A → createHiddenSpace() will succeed
  
  // Load PIN length from existing configuration (inherited from Space A)
  int pinLength = pinManager.getPinLength();
  LOG_INFO("Main", "Using PIN length from Space A: " + String(pinLength));
  
  bool setupComplete = false;
  
  while (!setupComplete) {
    // Request new PIN for hidden space
    displayManager.init();
    displayManager.showMessage("New Space PIN", 10, 20, true, 2);
    displayManager.showMessage("Must differ from", 10, 50, false, 1);
    displayManager.showMessage("main PIN", 10, 65, false, 1);
    delay(2000);
    
    // Request PIN entry using existing method
    String newPin = pinManager.requestPinInput("New Space PIN", false);
    
    if (newPin.isEmpty()) {
      // User cancelled
      displayManager.init();
      displayManager.showMessage("Setup Cancelled", 10, 30, true, 2);
      delay(2000);
      secureWipeString(spaceAPin);
      secureWipeString(newPin);
      secureRestart();
      return;
    }
    
    // Confirm PIN
    String confirmPin = pinManager.requestPinInput("Confirm PIN", true);
    
    if (confirmPin != newPin) {
      displayManager.init();
      displayManager.showMessage("PINs Don't Match", 10, 30, true, 2);
      delay(2000);
      continue; // Try again
    }
    
    // Verify new PIN != Space A PIN by attempting to decrypt slot A
    LOG_INFO("Main", "Verifying new PIN differs from Space A PIN");
    
    // We need to check if this PIN can decrypt slot A
    // Use a temporary check via tryDecryptSlot
    CryptoManager& crypto = CryptoManager::getInstance();
    
    // Open device.key and try to decrypt slot A with the new PIN
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r");
    if (!keyFile) {
      LOG_ERROR("Main", "Failed to open device.key for PIN verification");
      displayManager.init();
      displayManager.showMessage("Setup Failed", 10, 30, true, 2);
      displayManager.showMessage("Cannot read key file", 10, 60, false, 1);
      delay(2000);
      continue;
    }
    
    size_t fileSize = keyFile.size();
    keyFile.close();
    
    bool pinMatchesSpaceA = false;
    
    if (fileSize == 256) {
      // Dual-slot format - try to decrypt slot A
      // We'll use a simple PBKDF2 + AES check without modifying crypto state
      uint8_t salt[16], iv[16], encrypted[48];
      
      keyFile = LittleFS.open(DEVICE_KEY_FILE, "r");
      keyFile.seek(0); // SLOT_A_OFFSET = 0
      keyFile.read(salt, 16);
      keyFile.read(iv, 16);
      keyFile.read(encrypted, 48);
      keyFile.close();
      
      // Derive key from new PIN
      uint8_t derived_key[32];
      mbedtls_md_context_t sha256_ctx;
      mbedtls_md_init(&sha256_ctx);
      mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
      
      int ret = mbedtls_pkcs5_pbkdf2_hmac(
          &sha256_ctx,
          (const unsigned char*)newPin.c_str(), newPin.length(),
          salt, 16,
          PBKDF2_ITERATIONS_PIN,
          32,
          derived_key
      );
      
      mbedtls_md_free(&sha256_ctx);
      
      if (ret == 0) {
        // Decrypt and check MAGIC
        uint8_t iv_copy[16];
        memcpy(iv_copy, iv, 16);
        
        uint8_t plaintext[48];
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_dec(&aes, derived_key, 256);
        mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 48, iv_copy, encrypted, plaintext);
        mbedtls_aes_free(&aes);
        
        // Check MAGIC bytes
        const uint8_t MAGIC[4] = {0xA3, 0x7F, 0x2C, 0x91};
        if (memcmp(plaintext, MAGIC, 4) == 0) {
          pinMatchesSpaceA = true;
        }
        
        // Secure cleanup
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(plaintext, sizeof(plaintext));
      }
    }
    
    if (pinMatchesSpaceA) {
      LOG_WARNING("Main", "New PIN matches Space A PIN - rejecting");
      displayManager.init();
      displayManager.showMessage("PIN Error!", 10, 30, true, 2);
      displayManager.showMessage("Must differ from", 10, 60, false, 1);
      displayManager.showMessage("main PIN", 10, 75, false, 1);
      delay(2500);
      continue; // Loop back to request new PIN
    }
    
    // ═══ DURESS PIN COLLISION CHECK ═══
    // Reject if Space B PIN matches duress PIN
    // This prevents a scenario where entering Space B PIN triggers duress wipe
    if (crypto.isDuressPinConfigured() && crypto.verifyDuressPin(newPin)) {
      LOG_WARNING("Main", "New Space B PIN matches duress PIN - rejecting");
      displayManager.init();
      displayManager.showMessage("PIN Error!", 10, 30, true, 2);
      displayManager.showMessage("Matches duress PIN", 10, 60, false, 1);
      displayManager.showMessage("Choose different", 10, 75, false, 1);
      delay(2500);
      continue; // Loop back to request new PIN
    }
    
    // PIN is different - proceed with secondary slot creation
    LOG_INFO("Main", "Creating secondary slot with new PIN");
    
    if (crypto.createHiddenSpace(newPin)) {
      LOG_INFO("Main", "Secondary slot created successfully");
      displayManager.init();
      displayManager.showMessage("Secondary Slot Ready", 10, 30, true, 2);
      displayManager.showMessage("Rebooting...", 10, 60, false, 1);
      delay(2000);
      secureWipeString(spaceAPin);
      secureWipeString(newPin);
      secureWipeString(confirmPin);
      secureRestart();
      return; // Never reached
    } else {
      LOG_ERROR("Main", "Failed to create secondary slot");
      displayManager.init();
      displayManager.showMessage("Setup Failed", 10, 30, true, 2);
      displayManager.showMessage("Try again", 10, 60, false, 1);
      delay(2000);
      // Loop back to try again
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// IMPORT/EXPORT RESTRICTED MODE
// ═══════════════════════════════════════════════════════════════════════════

void runImportExportRestrictedMode() {
  LOG_INFO("Main", "Entering Import/Export restricted mode");
  
  // TOTP keys are held in KeyManager's internal vector, populated only
  // by begin()/loadKeys(). Normal boot calls keyManager.begin() in Phase 5,
  // which restricted mode branches off before reaching — without this call
  // exportKeysEncryptedRestricted() reads an empty, never-loaded vector
  // and silently produces a valid-looking but empty export file.
  // PasswordManager needs no equivalent call: getAllPasswordsForExport()
  // and replaceAllPasswords() both re-read/re-write disk directly,
  // independent of any internal vector state (confirmed by source review).
  keyManager.begin();
  
  // Generate a fresh AP password via the hardware-backed CTR_DRBG.
  // Never persisted to config.json, never reused across entries.
  auto generateRandomApPassword = []() -> String {
    const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // no ambiguous chars
    const int pwLen = 10;
    uint8_t randomBytes[pwLen];
    CryptoManager::getInstance().secureRandom(randomBytes, pwLen);
    String password;
    password.reserve(pwLen);
    for (int i = 0; i < pwLen; i++) {
      password += alphabet[randomBytes[i] % (sizeof(alphabet) - 1)];
    }
    return password;
  };
  
  String apName = "ESP32-IMPEXP-" + String((uint32_t)(esp_random() & 0xFFFF), HEX);
  String apPassword = generateRandomApPassword();
  
  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(apName.c_str(), apPassword.c_str());
  if (!apOk) {
    LOG_ERROR("Main", "Failed to start restricted-mode AP");
    ESP.restart();
    return;
  }

  DNSServer restrictedDnsServer;
  restrictedDnsServer.start(53, "*", WiFi.softAPIP());
  LOG_INFO("Main", "DNS Server started for restricted mode captive portal");
  
  // Display screen — pattern copied from the ordinary Admin AP info screen.
  // IMPORTANT: apPassword must never be written to LOG_INFO/LOG_DEBUG etc.
  // It is only ever shown on-device, never logged, never persisted.
  auto drawRestrictedModeScreen = [&]() {
    TFT_eSPI *tft = displayManager.getTft();
    auto *t = displayManager.getCurrentThemeColors();
    int W = tft->width();
    int H = tft->height();
    int cx = W / 2;
    
    tft->fillScreen(t->background_dark);
    tft->setTextDatum(MC_DATUM);
    
    tft->setTextSize(2);
    tft->setTextColor(t->accent_primary, t->background_dark);
    tft->drawString("Import/Export Mode", cx, 18);
    
    tft->drawFastHLine(20, 32, W - 40, t->text_secondary);
    
    tft->setTextSize(1);
    tft->setTextColor(t->text_secondary, t->background_dark);
    tft->drawString("Network", cx, 44);
    tft->setTextColor(t->text_primary, t->background_dark);
    tft->drawString(apName, cx, 56);
    
    tft->setTextColor(t->text_secondary, t->background_dark);
    tft->drawString("Password: " + apPassword +
                        "   IP: " + WiFi.softAPIP().toString(),
                    cx, 70);
    
    tft->setTextColor(t->text_secondary, t->background_dark);
    tft->drawString("Session ends automatically", cx, H - 20);
    
    // Button hints — match AP screen style
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    int btnL = (W - 224) / 2;
#else
    int btnL = 8;
#endif
    int btnR = btnL + 118;
    tft->fillRoundRect(btnL, H - 52, 106, 22, 6, t->background_light);
    tft->setTextColor(t->text_primary, t->background_light);
    tft->drawString("BTN1: WiFi QR", btnL + 53, H - 41);
    
    tft->fillRoundRect(btnR, H - 52, 106, 22, 6, t->accent_primary);
    tft->setTextColor(t->background_dark, t->accent_primary);
    tft->drawString("BTN2: Exit", btnR + 53, H - 41);
  };
  drawRestrictedModeScreen();
  
  auto highlightButtonRestricted = [&](int activeBtn) {
    TFT_eSPI *tft = displayManager.getTft();
    auto *t = displayManager.getCurrentThemeColors();
    int W = tft->width();
    int H = tft->height();
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    int btnL = (W - 224) / 2;
#else
    int btnL = 8;
#endif
    int btnR = btnL + 118;
    if (activeBtn == 1) {
      tft->fillRoundRect(btnL, H - 52, 106, 22, 6, t->accent_primary);
      tft->setTextDatum(MC_DATUM);
      tft->setTextColor(t->background_dark, t->accent_primary);
      tft->drawString("BTN1: WiFi QR", btnL + 53, H - 41);
      tft->fillRoundRect(btnR, H - 52, 106, 22, 6, t->background_light);
      tft->setTextColor(t->text_primary, t->background_light);
      tft->drawString("BTN2: Exit", btnR + 53, H - 41);
    } else {
      tft->fillRoundRect(btnL, H - 52, 106, 22, 6, t->background_light);
      tft->setTextDatum(MC_DATUM);
      tft->setTextColor(t->text_primary, t->background_light);
      tft->drawString("BTN1: WiFi QR", btnL + 53, H - 41);
      tft->fillRoundRect(btnR, H - 52, 106, 22, 6, t->accent_primary);
      tft->setTextColor(t->background_dark, t->accent_primary);
      tft->drawString("BTN2: Exit", btnR + 53, H - 41);
    }
  };
  
  webServerManager.startRestrictedImportExportServer();
  
  const unsigned long ROLLING_TIMEOUT = 10UL * 60 * 1000; // 10 min inactivity
  const unsigned long HARD_CAP        = 30UL * 60 * 1000; // 30 min absolute cap
  
  unsigned long startTime = millis();
  lastRestrictedModeActivity = millis();
  importExportCompleted = false;
  
  bool inQrMode = false;
  unsigned long qrStartTime = 0;
  int lastQrSecond = 30;
  String wifiQR = "WIFI:S:" + apName + ";T:WPA;P:" + apPassword + ";H:false;;";
  
  while (true) {
    esp_task_wdt_reset();
    restrictedDnsServer.processNextRequest();
    
    if (importExportCompleted) {
      LOG_INFO("Main", "Restricted mode: session closed by user");
      break;
    }
    if (millis() - lastRestrictedModeActivity > ROLLING_TIMEOUT) {
      LOG_INFO("Main", "Restricted mode: timeout - no activity");
      break;
    }
    if (millis() - startTime > HARD_CAP) {
      LOG_INFO("Main", "Restricted mode: hard cap reached");
      break;
    }
    
    if (inQrMode) {
      unsigned long now = millis();
      long elapsedMs = (long)(now - qrStartTime);
      long qrSecondsLeft = (30000L - elapsedMs) / 1000L;
      
      if (qrSecondsLeft <= 0) {
        // Auto-revert: QR window expired, return to main screen
        // without requiring a button press.
        displayManager.hideQRCode();
        inQrMode = false;
        drawRestrictedModeScreen();
        lastRestrictedModeActivity = millis();
        delay(50);
        continue;
      }
      
      if (qrSecondsLeft != lastQrSecond) {
        lastQrSecond = (int)qrSecondsLeft;
        displayManager.updateQRTimer((int)qrSecondsLeft);
      }
      
      if (readBtn1() || readBtn2()) {
        while (readBtn1() || readBtn2()) {
          esp_task_wdt_reset();
          delay(10);
        }
        displayManager.hideQRCode();
        inQrMode = false;
        drawRestrictedModeScreen();
        lastRestrictedModeActivity = millis();
      }
      delay(50);
      continue;
    }
    
    if (readBtn1()) {
      highlightButtonRestricted(1);
      while (readBtn1()) {
        esp_task_wdt_reset();
        delay(10);
      }
      delay(30);
      LOG_INFO("Main", "Restricted mode: showing WiFi QR");
      displayManager.showQRCode(wifiQR, 30);
      qrStartTime = millis();
      lastQrSecond = 30;
      inQrMode = true;
      lastRestrictedModeActivity = millis();
      continue;
    }
    
    if (readBtn2()) {
      highlightButtonRestricted(2);
      while (readBtn2()) {
        esp_task_wdt_reset();
        delay(10);
      }
      delay(30);
      LOG_INFO("Main", "Restricted mode: exit via BTN2");
      importExportCompleted = true;
      continue;
    }
    
    delay(100);
  }
  
  LOG_INFO("Main", "Exiting Import/Export restricted mode");
  delay(500); // let any in-flight HTTP response finish sending before AP teardown
  secureWipeString(apPassword);
  secureWipeString(wifiQR);
  WiFi.softAPdisconnect(true);
  ESP.restart();
}

// Глобальные переменные состояния
static AppMode currentMode = AppMode::TOTP;
static int currentKeyIndex = 0;
static int currentPasswordIndex = 0;
static int previousKeyIndex = -1;
static int previousPasswordIndex = -1;
static uint32_t previousPwRevision = 0;
unsigned long lastButtonPressTime = 0;
const int debounceDelay = 300;
const int factoryResetHoldTime = 5000;
const int powerOffHoldTime = 5000;
unsigned long lastActivityTime = 0;
bool isScreenOn = true;
bool isDimmed = false;
static const uint8_t DIM_BRIGHTNESS = 51; // 20% of 255

unsigned long bothButtonsPressStartTime = 0;
bool bleActionTriggered = false;
bool autoSendTriggered = false; // Автоматическая отправка пароля
bool autoSendDone = false;      // Флаг завершения автоотправки в текущей сессии
static bool hotpSavePending = false;
static int hotpSaveIndex = -1;

unsigned long lastBatteryCheckTime = 0;
const int batteryCheckInterval = 1000;
static int lastBatteryPercentage = -1;
static int _lastRealBatteryPercentage = 50;
static int chargeDisplayPct = -1;
static bool isCharging = false;

bool configPortalActive = false;
bool isFirstTimeSetup = false;  // true = первая настройка (нет credentials), false = fallback (есть credentials но не подключилось)
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
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  int wsYOff = (tft->height() - 80) / 2 - 25;
#else
  int wsYOff = 0;
#endif
  tft->setTextColor(colors->accent_primary, colors->background_dark);
  tft->setTextSize(2);
  tft->drawString("Web Server Started!", tft->width() / 2, 25 + wsYOff);

  // IP Address
  String ip = wifiManager.getIP();
  tft->setTextColor(colors->text_primary, colors->background_dark);
  tft->setTextSize(2);
  tft->drawString(ip, tft->width() / 2, 60 + wsYOff);

  // Domain - защита от краша
  tft->setTextColor(colors->accent_secondary, colors->background_dark);
  tft->setTextSize(1);
  String mdnsHostname = configManager.loadMdnsHostname();
  mdnsHostname += ".local";
  tft->drawString(mdnsHostname, tft->width() / 2, 85 + wsYOff);

  // Instructions
  tft->setTextColor(colors->text_secondary, colors->background_dark);
  tft->setTextSize(1);
  tft->drawString("Ready for connections", tft->width() / 2, 105 + wsYOff);

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

  while (readBtn1() && readBtn2()) {
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
      LittleFS.remove("/device_ble_pin.json.enc"); // <-- СБРОС Device BLE PIN
      LittleFS.remove("/duress_pin.hash"); // <-- СБРОС Duress PIN
      LittleFS.remove("/session.json.enc"); // <-- СБРОС СЕССИЙ И CSRF
      LittleFS.remove("/theme_pref.json"); // <-- СБРОС THEME PREF
      LittleFS.remove("/startup_pref.json"); // <-- СБРОС STARTUP PREF
      LittleFS.remove("/hid_mode.json"); // <-- СБРОС HID MODE PREF
      LittleFS.remove("/boot_mode.json"); // <-- СБРОС BOOT MODE PREF
      LittleFS.remove("/.setup_hidden_space"); // <-- СБРОС Hidden Space Setup Flag
      LittleFS.remove("/.setup_import_export"); // <-- СБРОС Import/Export Restricted Mode Flag

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
      
      // ═══ FORMAT LITTLEFS TO REMOVE ALL ORPHANED FILES ═══
      // This ensures Space B files (with HMAC-derived names) are completely removed
      // since we don't know the Space B device key to derive their paths
      LOG_INFO("Main", "Formatting LittleFS to ensure complete wipe...");
      LittleFS.end();
      LittleFS.format();
      LittleFS.begin(false);
      LOG_INFO("Main", "LittleFS formatted - all files removed");
      
      // ═══ SECURE MEMORY ZEROING ═══
      displayManager.showMessage("Done. Rebooting...", 10, 60);

      delay(2500);
      secureRestart();
    }

    int progress = (holdTime * 100) / factoryResetHoldTime;
    displayManager.showMessage("Resetting: " + String(progress) + "%", 10, 100);
    delay(100);
  }
  LOG_INFO("Main", "Factory reset aborted. Continuing boot");
  displayManager.init();
}

void setup() {
  esp_register_shutdown_handler(panicShutdown);
  Serial.begin(115200);
#ifdef ARDUINO_USB_CDC_ON_BOOT
  // S3 native USB: don't block boot if no USB host connected
  { uint32_t _t = millis(); while (!Serial && (millis() - _t) < 1500) { delay(10); } }
#endif
  LogManager::getInstance().begin();
  LOG_INFO("Main", "T-Disp-TOTP Booting Up");

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  if (readBtn1() && readBtn2()) {
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
  if (!LittleFS.begin(false)) {
    // LittleFS.begin(format_if_failed) may not handle LFS_ERR_CORRUPT.
    // Explicitly format and retry once.
    LOG_WARNING("Main", "LittleFS mount failed, formatting and retrying...");
    LittleFS.format();
    if (!LittleFS.begin(false)) {
      LOG_CRITICAL("Main", "LittleFS Mount Failed!");
      DisplayManager tempDisplay;
      tempDisplay.init();
      tempDisplay.showMessage("LittleFS Failed", 10, 30, true);
      while (1)
        ;
    }
  }

  LOG_INFO("Main", "Loading display rotation for button helpers...");
  g_displayRotation = configManager.getDisplayRotation();
  LOG_INFO("Main", "Display rotation loaded: " + String(g_displayRotation));

  // ═══ CHECK FOR SECONDARY SLOT SETUP FLAG ═══
  if (LittleFS.exists("/.setup_hidden_space")) {
    LOG_INFO("Main", "Secondary slot setup flag detected - starting setup flow");
    LittleFS.remove("/.setup_hidden_space");
    runHiddenSpaceSetupFlow();
    // runHiddenSpaceSetupFlow() does not return — it reboots on completion
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
    
    // ═══ SPACE CONTEXT LOGGING ═══
    // Space context is already set inside CryptoManager::unlockDeviceKeyWithPin()
    // via initSpacePaths(). Log active space for debugging:
    ActiveSpace activeSpace = CryptoManager::getInstance().getActiveSpace();
    LOG_INFO("BOOT", "Active space: " + String(activeSpace == ActiveSpace::A ? "A" : 
                                                activeSpace == ActiveSpace::B ? "B" : "NONE"));

    // ═══ CHECK FOR IMPORT/EXPORT RESTRICTED MODE FLAG ═══
    // Only reached after successful PIN unlock (not first-boot, not legacy,
    // not lockout) — _activeSpace and DRBG are guaranteed initialized here.
    if (LittleFS.exists("/.setup_import_export")) {
      File flagFile = LittleFS.open("/.setup_import_export", "r");
      String requestedSpace = flagFile ? flagFile.readString() : "";
      if (flagFile) flagFile.close();
      requestedSpace.trim();

      ActiveSpace currentSpaceCheck = CryptoManager::getInstance().getActiveSpace();
      String currentSpaceStr = (currentSpaceCheck == ActiveSpace::B) ? "B" : "A";

      if (requestedSpace == currentSpaceStr) {
        LOG_INFO("Main", "Import/Export restricted mode flag detected for active space " + currentSpaceStr);
        LittleFS.remove("/.setup_import_export");
        runImportExportRestrictedMode();
        // does not return — reboots on completion
      } else {
        LOG_INFO("Main", "Import/Export flag exists for space " + requestedSpace +
                          " but active space is " + currentSpaceStr +
                          " — skipping, flag left intact for its own space's next unlock");
      }
    }

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
  Theme savedTheme;
  
  // Try per-space theme file first
  String themePath = CryptoManager::getInstance().getSpacePath("theme");
  if (LittleFS.exists(themePath)) {
      fs::File tf = LittleFS.open(themePath, "r");
      if (tf) {
          JsonDocument tdoc;
          if (deserializeJson(tdoc, tf) == DeserializationError::Ok) {
              String themeStr = tdoc["theme"] | "";
              if      (themeStr == "dark")  savedTheme = Theme::DARK;
              else if (themeStr == "light") savedTheme = Theme::LIGHT;
              else                          savedTheme = configManager.loadTheme();
          } else {
              savedTheme = configManager.loadTheme();
          }
          tf.close();
      } else {
          savedTheme = configManager.loadTheme();
      }
  } else {
      // First boot into this space — use default per space
      if (CryptoManager::getInstance().getActiveSpace() == ActiveSpace::B) {
          savedTheme = Theme::DARK;  // Space B default — never inherit from Space A
      } else {
          savedTheme = configManager.loadTheme();  // Space A uses global config
      }
  }
  
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

  String startupMode;
  
  // Try per-space startup mode file first
  String startupPath = CryptoManager::getInstance().getSpacePath("startup_mode");
  if (LittleFS.exists(startupPath)) {
      fs::File sf = LittleFS.open(startupPath, "r");
      if (sf) {
          JsonDocument sdoc;
          if (deserializeJson(sdoc, sf) == DeserializationError::Ok) {
              startupMode = sdoc["mode"] | "";
              if (startupMode.length() == 0) {
                  startupMode = configManager.getStartupMode();
              }
          } else {
              startupMode = configManager.getStartupMode();
          }
          sf.close();
      } else {
          startupMode = configManager.getStartupMode();
      }
  } else {
      // Default per space: Space A → from global config,
      // Space B → passwords (often used as real vault)
      bool isSpaceB = (CryptoManager::getInstance().getActiveSpace()
                       == ActiveSpace::B);
      startupMode = isSpaceB ? "passwords" : configManager.getStartupMode();
  }
  
  LOG_INFO("Main", "Loaded startup mode: " + startupMode);
  if (startupMode == "password" || startupMode == "passwords") {
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
  
  // Try per-space file first
  String savedBootMode;
  String bootPath = CryptoManager::getInstance().getSpacePath("boot_mode");
  if (LittleFS.exists(bootPath)) {
    fs::File bf = LittleFS.open(bootPath, "r");
    if (bf) {
      JsonDocument bdoc;
      if (deserializeJson(bdoc, bf) == DeserializationError::Ok) {
        savedBootMode = bdoc["mode"] | "";
      }
      bf.close();
    }
  }
  
  // Fallback: Space A uses global config, Space B defaults to "wifi"
  if (savedBootMode.length() == 0) {
    if (CryptoManager::getInstance().getActiveSpace() == ActiveSpace::B) {
      savedBootMode = "wifi";  // Space B default
    } else {
      savedBootMode = configManager.getBootMode();  // Space A fallback to global config
    }
  }
  
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
#ifdef BOARD_HAS_USB_HID
  defaultHidIsBle = (configManager.getDefaultHidMode() != "usb");
#endif
  if (rtcManager.getConfig().enabled) {
    if (rtcManager.init()) {
      if (rtcManager.syncFromRTC()) {
        String savedTz = configManager.getTimezone();
        if (savedTz.length() > 0) {
          setenv("TZ", savedTz.c_str(), 1);
          tzset();
        }
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
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
      int btnL = (W - 224) / 2;
#else
      int btnL = 8;
#endif
      int btnR = btnL + 118;
      tft->fillRoundRect(btnL, H - 52, 106, 22, 6, t->background_light);
      tft->setTextColor(t->text_primary, t->background_light);
      tft->drawString("BTN1: WiFi QR", btnL + 53, H - 41);

      // BTN2 hint (right side, like Offline button in mode selection)
      tft->fillRoundRect(btnR, H - 52, 106, 22, 6, t->accent_primary);
      tft->setTextColor(t->background_dark, t->accent_primary);
      tft->drawString("BTN2: Continue", btnR + 53, H - 41);
    };

    drawApInfoScreen();

    auto highlightButton = [&](int activeBtn) {
      TFT_eSPI *tft = displayManager.getTft();
      auto *t = displayManager.getCurrentThemeColors();
      int W = tft->width();
      int H = tft->height();
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
      int btnL = (W - 224) / 2;
#else
      int btnL = 8;
#endif
      int btnR = btnL + 118;
      // BTN1
      if (activeBtn == 1) {
        tft->fillRoundRect(btnL, H - 52, 106, 22, 6, t->accent_primary);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(t->background_dark, t->accent_primary);
        tft->drawString("BTN1: WiFi QR", btnL + 53, H - 41);
        tft->fillRoundRect(btnR, H - 52, 106, 22, 6, t->background_light);
        tft->setTextColor(t->text_primary, t->background_light);
        tft->drawString("BTN2: Continue", btnR + 53, H - 41);
      } else {
        tft->fillRoundRect(btnL, H - 52, 106, 22, 6, t->background_light);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(t->text_primary, t->background_light);
        tft->drawString("BTN1: WiFi QR", btnL + 53, H - 41);
        tft->fillRoundRect(btnR, H - 52, 106, 22, 6, t->accent_primary);
        tft->setTextColor(t->background_dark, t->accent_primary);
        tft->drawString("BTN2: Continue", btnR + 53, H - 41);
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

        if (readBtn1() || readBtn2()) {
          while (readBtn1() || readBtn2()) {
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
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
        int autoY = tft->height() - 70;
#else
        int autoY = 122;
#endif
        tft->fillRect(0, autoY - 6, tft->width(), 14, TFT_BLACK);
        tft->drawString("Auto: " + String(secondsLeft) + "s", tft->width() / 2,
                        autoY);
      }

      if (readBtn1()) {
        highlightButton(1);
        while (readBtn1()) {
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

      if (readBtn2()) {
        highlightButton(2);
        while (readBtn2()) {
          esp_task_wdt_reset();
          delay(10);
        }
        delay(30);
        esp_task_wdt_reset();
        break;
      }

      // Auto-exit prompt when a client connects to AP
      if (WiFi.softAPgetStationNum() > 0) {
        LOG_INFO("Main", "Client connected to AP — skipping prompt");
        break;
      }

      delay(50);
    }

    // Автозапуск веб-сервера в AP режиме
    LOG_INFO("Main", "Auto-starting Web Server in AP Mode...");
    webServerManager.start();

    if (displayManager.isQRCodeActive())
      displayManager.hideQRCode();
    displayManager.clearMessageArea(0, 0, 240, 135);
    displayManager.setKeySwitched(true); // Clear content area after AP prompt
    previousKeyIndex = -1; // Force TOTP display redraw after AP prompt

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
        displayManager.clearMessageArea(0, 0, 240, 135);
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
    displayManager.clearMessageArea(0, 0, 240, 135);

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
      // Проверяем, есть ли сохраненные credentials
      // Если нет - это первая настройка, если есть - это fallback после неудачного подключения
      bool hasCredentials = LittleFS.exists(WIFI_CONFIG_FILE) || LittleFS.exists(WIFI_CONFIG_FILE_LEGACY);
      
      if (hasCredentials) {
        LOG_WARNING("Main", "WiFi connection failed with saved credentials. Starting fallback config portal...");
        isFirstTimeSetup = false;  // Это fallback - credentials есть, но подключение не удалось
      } else {
        LOG_WARNING("Main", "No WiFi credentials found. Starting first-time config portal...");
        isFirstTimeSetup = true;  // Это первая настройка - credentials нет
      }

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
        String savedTz = configManager.getTimezone();
        if (savedTz.length() > 0) {
          setenv("TZ", savedTz.c_str(), 1);
          tzset();
        }
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
  displayManager.clearMessageArea(0, 0, 240, 60);

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

inline bool isInBlePipeline() {
  bool inPipeline = currentMode == AppMode::BLE_ADVERTISING ||
                    currentMode == AppMode::BLE_PIN_ENTRY ||
                    currentMode == AppMode::BLE_CONFIRM_SEND;
#ifdef BOARD_HAS_USB_HID
  inPipeline = inPipeline || currentMode == AppMode::USB_HID_SEND;
#endif
  return inPipeline;
}

void handleButtons() {
  esp_task_wdt_reset();
  static unsigned long button1PressStartTime = 0;
  static unsigned long button2PressStartTime = 0;
  static bool suppressNextSinglePress = false;
  bool buttonPressed = false;

  // readBtn1() and readBtn2() already handle rotation, so use them directly
  bool button1_is_pressed = readBtn1();
  bool button2_is_pressed = readBtn2();

  // --- Логика двойного нажатия (высший приоритет) ---
  if (button1_is_pressed && button2_is_pressed) {
    // Если зажаты обе кнопки, сбрасываем таймеры одиночных нажатий, чтобы
    // предотвратить конфликт
    button1PressStartTime = 0;
    button2PressStartTime = 0;

    // HOTP generation: both buttons held 1 second in TOTP mode
    if (currentMode == AppMode::TOTP) {
      const auto& keys = keyManager.getAllKeys();
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
            previousPasswordIndex = -1;
#ifdef BOARD_HAS_USB_HID
            {
              // Check Device BLE PIN before showing HID prompt
              bool pinOk = true;
              if (CryptoManager::getInstance().isDeviceBlePinEnabled()) {
                LOG_INFO("Main", "Device BLE PIN required before HID prompt");
                pinOk = pinManager.requestDeviceBlePinForTransmission();
              }
              if (!pinOk) {
                LOG_WARNING("Main", "Device BLE PIN failed or cancelled. Aborting HID prompt");
                bleActionTriggered = false;
                displayManager.hideLoader();
              } else {
                bool useBle = displayManager.drawHidPrompt(defaultHidIsBle);
                wipeWildcardSession(); // force fresh generation for this HID session
                if (useBle) {
                  currentMode = AppMode::BLE_ADVERTISING;
                  LOG_INFO("Main", "HID prompt: BLE selected");
                } else {
                  currentMode = AppMode::USB_HID_SEND;
                  LOG_INFO("Main", "HID prompt: USB selected");
                }
              }
            }
#else
            wipeWildcardSession(); // force fresh generation for this HID session
            currentMode = AppMode::BLE_ADVERTISING;
            LOG_INFO("Main", "Both buttons held. Switching to BLE_ADVERTISING mode");
#endif
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
      autoSendDone = false;
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
      if (isInBlePipeline()) {
        bleKeyboardManager.end();
        bleActionTriggered = false;
        autoSendDone = false;
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
            (currentMode == AppMode::TOTP) ? "Passwords..." : "TOTP/HOTP...";
        displayManager.drawGenericLoader(progress, loaderText);
      }
    }
  } else {
    if (button1PressStartTime > 0) {
      displayManager.hideLoader();
      if (millis() - button1PressStartTime < powerOffHoldTime) {
        LOG_DEBUG("Main", "Button 1 press: Previous item");
        if (currentMode == AppMode::TOTP) {
          const auto& keys = keyManager.getAllKeys();
          if (!keys.empty()) {
            currentKeyIndex =
                (currentKeyIndex == 0) ? keys.size() - 1 : currentKeyIndex - 1;
            displayManager.setKeySwitched(true); // <-- ADDED
            buttonPressed = true;
          }
        } else if (currentMode == AppMode::PASSWORD) {
          const auto& passwords = passwordManager.getAllPasswords();
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
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
      esp_sleep_enable_ext1_wakeup((1ULL << GPIO_NUM_0), ESP_EXT1_WAKEUP_ANY_LOW);
#else
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
#endif
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
          const auto& keys = keyManager.getAllKeys();
          if (!keys.empty()) {
            currentKeyIndex = (currentKeyIndex + 1) % keys.size();
            displayManager.setKeySwitched(true); // <-- ADDED
            buttonPressed = true;
          }
        } else if (currentMode == AppMode::PASSWORD) {
          const auto& passwords = passwordManager.getAllPasswords();
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
      isDimmed = false;
    } else if (isDimmed) {
      LOG_DEBUG("Main", "Button press restored brightness from dim");
      displayManager.setBrightness(255);
      isDimmed = false;
    }
    previousKeyIndex = -1;
    previousPasswordIndex = -1;
  }
}

// Функция для проверки нажатия кнопок и включения экрана
void checkScreenWakeup() {
  static bool button1PreviousState = HIGH;
  static bool button2PreviousState = HIGH;

  // readBtn1/readBtn2 return true when pressed (LOW), false when not pressed (HIGH)
  // So we need to invert for state comparison
  bool button1Current = readBtn1() ? LOW : HIGH;
  bool button2Current = readBtn2() ? LOW : HIGH;

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
      isDimmed = false;
    } else if (isDimmed) {
      LOG_DEBUG("Main", "Button press restored brightness from dim in BLE mode");
      displayManager.setBrightness(255);
      isDimmed = false;
    }
  }

  button1PreviousState = button1Current;
  button2PreviousState = button2Current;
}

void loop() {
  if (configPortalActive) {
    // ✅ Обработка BUTTON_2 для retry WiFi подключения
    // ТОЛЬКО в fallback режиме (когда credentials уже есть)
    if (!isFirstTimeSetup) {
      static bool button2PreviousState = HIGH;
      static bool retryInProgress = false;
      // readBtn2() returns true when pressed (LOW), false when not pressed (HIGH)
      bool button2Current = readBtn2() ? LOW : HIGH;
      
      // Детект нажатия (переход HIGH → LOW)
      if (button2PreviousState == HIGH && button2Current == LOW && !retryInProgress) {
        delay(50);  // Debounce
        
        if (readBtn2()) {
          retryInProgress = true;
          LOG_INFO("Main", "BUTTON_2 pressed - retrying WiFi connection (fallback mode)");
          
          // Показываем сообщение
          displayManager.init();
          displayManager.showMessage("Retrying WiFi...", 10, 30, false, 2);
          displayManager.showMessage("Please wait...", 10, 60, false, 1);
          delay(1000);
          
          // Пытаемся подключиться (40 попыток, 20 секунд)
          bool connected = wifiManager.connect();
          
          if (connected) {
            // ✅ УСПЕХ - выходим из config portal
            LOG_INFO("Main", "WiFi connected! IP: " + wifiManager.getIP());
            
            // Останавливаем config portal
            webServerManager.stop();
            wifiManager.stopConfigPortal();
            
            // Включаем watchdog обратно
            esp_task_wdt_init(30, true);
            esp_task_wdt_add(NULL);
            LOG_INFO("Main", "Watchdog re-enabled after successful WiFi connection");
            
            // Сбрасываем флаги
            configPortalActive = false;
            isFirstTimeSetup = false;
            
            // Показываем успех
            displayManager.init();
            displayManager.showMessage("WiFi Connected!", 10, 30, false, 2);
            displayManager.showMessage("IP: " + wifiManager.getIP(), 10, 60, false, 1);
            delay(2000);
            displayManager.init();
            
            // Продолжаем нормальную работу (выходим из loop, дальше обычная логика)
            retryInProgress = false;
            return;
            
          } else {
            // ❌ НЕУДАЧА - восстанавливаем config portal
            LOG_WARNING("Main", "WiFi retry failed - restoring config portal");
            
            // Восстанавливаем AP режим
            WiFi.mode(WIFI_AP);
            WiFi.softAP("ESP32-TOTP-Setup");
            LOG_INFO("Main", "Access Point restored");
            
            // DNS продолжает работать, не нужно перезапускать
            
            // Показываем сообщение о неудаче
            displayManager.init();
            displayManager.showMessage("Connection Failed!", 10, 10, true, 2);
            delay(1500);
            
            // Восстанавливаем экран config portal
            displayManager.init();
            displayManager.showMessage("WiFi Setup Mode", 10, 10, false, 2);
            displayManager.showMessage("1. Connect to WiFi:", 10, 40);
            displayManager.showMessage("ESP32-TOTP-Setup", 15, 60, false, 2);
            displayManager.showMessage("2. Go to 192.168.4.1", 10, 90);
            
            retryInProgress = false;
          }
        }
      }
      
      button2PreviousState = button2Current;
    }
    // else: первая настройка - кнопка BUTTON_2 не обрабатывается
    
    // ✅ EXISTING: Process DNS
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
    previousKeyIndex = -1;
    previousPasswordIndex = -1;
    displayManager.resetLoaderState();
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

      // Startup PIN cannot be disabled while Hidden Space exists — it must
      // be explicitly removed first (requires Space B's own PIN).
      if (CryptoManager::getInstance().isHiddenSpaceProvisioned()) {
        LOG_WARNING("Main", "PIN disable blocked: Hidden Space is still provisioned");
        displayManager.init();
        displayManager.showMessage("Cannot Disable PIN", 10, 20, true, 2);
        displayManager.showMessage("Hidden Space active", 10, 45, false, 1);
        displayManager.showMessage("Remove it first", 10, 65, false, 1);
        delay(3000);
        displayManager.init();
        return;
      }

      if (CryptoManager::getInstance().disablePinEncryption(enteredPin)) {
        LOG_INFO("Main", "PIN protection disabled via device confirmation");

        displayManager.init();
        displayManager.showMessage("PIN Disabled!", 10, 30, false, 2);
        displayManager.showMessage("Rebooting...", 10, 60, false, 2);
        delay(2000);

        // Перезагружаемся
        secureRestart();
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

  if (shouldPromptRemoveHiddenSpace) {
    shouldPromptRemoveHiddenSpace = false;

    LOG_INFO("Main", "Hidden Space removal request detected - prompting on device");

    if (!isScreenOn) {
      displayManager.turnOn();
      isScreenOn = true;
    }

    displayManager.init();
    displayManager.showMessage("Web Request:", 10, 20, false, 2);
    displayManager.showMessage("Remove Hidden Space?", 10, 40, false, 2);
    displayManager.showMessage("Enter Space B PIN", 10, 70, false, 1);
    delay(2000);

    String spaceBPin = pinManager.requestPinInput("Confirm Space B PIN");

    if (spaceBPin.length() > 0) {
      displayManager.init();
      displayManager.showMessage("Removing...", 10, 30, false, 2);

      if (CryptoManager::getInstance().removeHiddenSpaceWithPin(spaceBPin)) {
        LOG_INFO("Main", "Hidden Space removed via device confirmation");
        displayManager.init();
        displayManager.showMessage("Hidden Space", 10, 20, false, 2);
        displayManager.showMessage("Removed!", 10, 45, false, 2);
        displayManager.showMessage("Rebooting...", 10, 70, false, 1);
        delay(2000);
        secureRestart();
      } else {
        LOG_ERROR("Main", "Failed to remove Hidden Space - wrong PIN");
        displayManager.init();
        displayManager.showMessage("Wrong PIN!", 10, 30, true, 2);
        displayManager.showMessage("Try again via web", 10, 60, false, 1);
        delay(3000);
      }
    } else {
      LOG_INFO("Main", "Hidden Space removal cancelled by user");
    }
  }

  // Проверяем таймаут API веб-сервера и самого сервера
  if (webServerManager.isRunning()) {
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
    if (!readBtn1() && !readBtn2()) {
      delay(50); // stabilization
      esp_task_wdt_reset();

      // Now do all heavy work from stable main loop context
      keyManager.getKeyRef(hotpSaveIndex).counter++;
      const auto& updatedKeys = keyManager.getAllKeys();
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
    if (isDimmed) {
      displayManager.setBrightness(255);
      isDimmed = false;
    }
  }

  // Independent auto lock timer — only when screen timeout is Never (0)
  // When screen timeout > 0, auto lock runs inside pseudo-sleep polling loop
  uint32_t autoLockSecondsMain = configManager.getAutoLockTimeout();
  if (!webServerManager.isRunning() && screenTimeoutSeconds == 0 &&
      !bleKeyboardManager.isConnected() &&
      !isInBlePipeline() &&
#ifdef BOARD_HAS_USB_HID
      !usbHidManager.isConnected() &&
#endif
      autoLockSecondsMain > 0 &&
      (millis() - lastActivityTime > (autoLockSecondsMain * 1000UL))) {
    LOG_INFO("Main", "Auto lock timeout reached (screen=Never mode). Entering deep sleep.");
    displayManager.turnOff();
    secureShutdown();
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    esp_sleep_enable_ext1_wakeup((1ULL << GPIO_NUM_0), ESP_EXT1_WAKEUP_ANY_LOW);
#else
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
#endif
    esp_deep_sleep_start();
  }

  // Dim check — fires before screen timeout
  uint16_t dimTimeoutSeconds = configManager.getDimTimeout();
  bool canApplyIdleEffect = !webServerManager.isRunning() &&
      isScreenOn &&
      !bleKeyboardManager.isConnected() &&
      !isInBlePipeline() &&
#ifdef BOARD_HAS_USB_HID
      !usbHidManager.isConnected() &&
#endif
      true;

  if (canApplyIdleEffect && dimTimeoutSeconds > 0 && !isDimmed &&
      (millis() - lastActivityTime > (dimTimeoutSeconds * 1000UL))) {
      LOG_INFO("Main", "Dim timeout reached. Reducing brightness to 20%.");
      displayManager.setBrightness(DIM_BRIGHTNESS);
      isDimmed = true;
  }

  if (canApplyIdleEffect && screenTimeoutSeconds > 0 &&
      (millis() - lastActivityTime > (screenTimeoutSeconds * 1000UL))) {

    // Веб-сервер не активен - можно засыпать
    LOG_INFO(
        "Main",
        "Screen timeout reached. Web server inactive. Entering pseudo-sleep.");

    // Отключаем BLE для безопасности, если он активен
    if (isInBlePipeline()) {
      LOG_INFO("Main", "Disabling BLE due to screen timeout for security");
      bleKeyboardManager.end();
      currentMode = AppMode::PASSWORD;
      bleActionTriggered = false;
      autoSendDone = false;
    }

    // 1. Выключаем дисплей перед сном
    if (!isInBlePipeline()) {
        displayManager.turnOff();
    }
    isScreenOn = false;

    // 2. Pseudo-sleep: lower CPU frequency, poll buttons
    // esp_light_sleep_start() causes POWER_ON reset on battery due to
    // high internal resistance — voltage drops below RTC threshold on CPU wake spike.
    // Hardware-incompatible without bulk capacitor on power rail.
    uint32_t autoLockSeconds = configManager.getAutoLockTimeout();
    LOG_INFO("Main", "Entering pseudo-sleep (40MHz CPU, display off, TFT sleep). AutoLock=" + String(autoLockSeconds) + "s");
    
#ifndef ARDUINO_USB_CDC_ON_BOOT
    Serial.flush();
#endif
    setCpuFrequencyMhz(40);
#ifndef ARDUINO_USB_CDC_ON_BOOT
    Serial.updateBaudRate(115200);
#endif
    
    unsigned long pseudoSleepStart = millis();
    
    // Poll both buttons — wake on any press, or auto lock on timeout
    while (!readBtn1() && !readBtn2()) {
      esp_task_wdt_reset();
      
      if (autoLockSeconds > 0 &&
          (millis() - pseudoSleepStart > (autoLockSeconds * 1000UL))) {
        setCpuFrequencyMhz(240);
#ifndef ARDUINO_USB_CDC_ON_BOOT
        Serial.updateBaudRate(115200);
#endif
        LOG_INFO("Main", "Auto lock timeout reached. Entering deep sleep.");
        displayManager.turnOff();
        secureShutdown();
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
        esp_sleep_enable_ext1_wakeup((1ULL << GPIO_NUM_0), ESP_EXT1_WAKEUP_ANY_LOW);
#else
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
#endif
        esp_deep_sleep_start();
        // Never reaches here
      }
      
      delay(50);
    }
    
    // Wait for button release before resuming
    while (readBtn1() || readBtn2()) {
      esp_task_wdt_reset();
      delay(10);
    }
    
    // Exit pseudo-sleep: restore full CPU speed
    setCpuFrequencyMhz(240);
#ifndef ARDUINO_USB_CDC_ON_BOOT
    Serial.updateBaudRate(115200);
#endif
    
    LOG_INFO("Main", "Woke up from pseudo-sleep.");
    lastActivityTime = millis();
    if (!isScreenOn) {
      displayManager.turnOn(); // sends TFT_SLPOUT + 120ms delay + backlight
      isScreenOn = true;
    }
    isDimmed = false; // Always reset dim state after wake from pseudo-sleep
    // Re-sync system clock from DS3231 after wake
    if (rtcManager.getConfig().enabled && rtcManager.isAvailable()) {
      delay(5);
      rtcManager.syncFromRTC();
      String savedTz = configManager.getTimezone();
      if (savedTz.length() > 0) {
        setenv("TZ", savedTz.c_str(), 1);
        tzset();
      }
    }
  }

  // Вне if (isScreenOn) — BLE_PIN_ENTRY требует экран принудительно 
  if (currentMode == AppMode::BLE_PIN_ENTRY && !isScreenOn) { 
      LOG_INFO("MAIN", "BLE_PIN_ENTRY: screen was off - forcing wakeup"); 
      displayManager.turnOn(); 
      isScreenOn = true; 
      lastActivityTime = millis(); 
  } 

  if (isScreenOn) {
    // Пропускаем обновления если активен лоадер или QR код
#ifdef BOARD_HAS_USB_HID
    bool _isUsbHidActive = (currentMode == AppMode::USB_HID_SEND);
#else
    bool _isUsbHidActive = false;
#endif
    if (!displayManager.isLoaderActive() && !displayManager.isQRCodeActive() && !_isUsbHidActive) {
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
        if (freeHeap < 30000) { // Меньше 30KB - критично!
          LOG_CRITICAL("Memory",
                       "CRITICAL LOW MEMORY! Device may become unstable");

          // Принудительная очистка кэшей при критической нехватке памяти
          if (freeHeap < 20000) {
            ESP.restart(); // Аварийная перезагрузка при < 20KB
          }
        }
      }
    }

    // Force full redraw after rotation change (e.g. triggered from web UI)
    if (displayManager.needsFullRedraw()) {
        previousKeyIndex = -1;
        previousPasswordIndex = -1;
        displayManager.clearFullRedrawFlag();
    }

    switch (currentMode) {
    case AppMode::TOTP: {
      const auto& keys = keyManager.getAllKeys();
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
                tft->fillRect(
                    0, 115, tft->width(), 60,
                    displayManager.getCurrentThemeColors()->background_dark);

                tft->setTextDatum(MC_DATUM);
                tft->setTextColor(
                    displayManager.getCurrentThemeColors()->text_secondary,
                    displayManager.getCurrentThemeColors()->background_dark);
                tft->setTextSize(1);
                tft->drawString("⚠️ Connect to network", tft->width() / 2, 120);
                tft->drawString("for time sync", tft->width() / 2, 135);
                tft->drawString("or switch to passwords", tft->width() / 2,
                                150);
                tft->drawString("(Hold BTN1)", tft->width() / 2, 165);
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
      const auto& passwords = passwordManager.getAllPasswords();
      if (!passwords.empty()) {
        uint32_t pwRevision = passwordManager.getRevision();
        if (currentPasswordIndex != previousPasswordIndex || pwRevision != previousPwRevision || displayManager.consumePasswordRedrawFlag()) {
          previousPwRevision = pwRevision;
          auto& curPwd = passwords[currentPasswordIndex];
          // Check for duplicate pw_hash among all entries
          bool isDup = false;
          if (!curPwd.pw_hash.isEmpty()) {
            for (int pi = 0; pi < (int)passwords.size(); pi++) {
              if (pi != currentPasswordIndex &&
                  passwords[pi].pw_hash == curPwd.pw_hash) {
                isDup = true;
                break;
              }
            }
          }
          
          // PIN: only digits
          bool isPin = false;
          if (curPwd.password.length() > 0) {
            isPin = true;
            for (char c : curPwd.password) {
              if (c < '0' || c > '9') { isPin = false; break; }
            }
          }
          
          // NAME: password contains entry name (case-insensitive)
          bool isName = false;
          if (curPwd.password.length() > 0 && curPwd.name.length() > 0) {
            String pwLower = curPwd.password;
            pwLower.toLowerCase();
            String nameLower = curPwd.name;
            nameLower.toLowerCase();
            // Split name by non-alphanumeric separators, check each word >= 3 chars
            String word = "";
            for (int ci = 0; ci <= (int)nameLower.length(); ci++) {
              char c = (ci < (int)nameLower.length()) ? nameLower[ci] : 0;
              if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                word += c;
              } else {
                if (word.length() >= 3 && pwLower.indexOf(word) >= 0) {
                  isName = true;
                  break;
                }
                word = "";
              }
            }
          }
          
          displayManager.drawPasswordLayout(
              curPwd.name,
              curPwd.password,
              chargeDisplayPct,
              isCharging,
              webServerManager.isRunning(),
              curPwd.strength,
              isDup,
              isPin,
              isName,
              curPwd.getAutoSend(),
              curPwd.getSendLogin(),
              curPwd.nav_mode,
              curPwd.getWildcard());
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
      static bool connectionTimerRestarted = false;

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
            String _pinRaw = String(bleKeyboardManager.getStaticPIN());
            while (_pinRaw.length() < 6) _pinRaw = "0" + _pinRaw;
            String pinMsg = "PIN: " + _pinRaw;
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
        connectionTimerRestarted = false;
        LOG_INFO("Main", "BLE Keyboard started. Waiting for connection...");
        }
      }

      if (bleKeyboardManager.isConnected()) {
        // Сброс таймера ТОЛЬКО ОДИН РАЗ при подключении
        if (currentMode == AppMode::BLE_ADVERTISING && !bleKeyboardManager.isSecure()) {
          if (!connectionTimerRestarted) {
            bleStartTime = millis();
            connectionTimerRestarted = true;
            LOG_INFO("Main", "Device connected - timer restarted for authentication (30s)");
          }
        }

        if (bleKeyboardManager.isSecure()) {
          LOG_INFO("Main", "BLE secure connection established");
          currentMode = AppMode::BLE_PIN_ENTRY;
          lastActivityTime = millis(); // Reset timeout on PIN entry transition
          bleInitialized = false; // Reset for next time
          bleStartTime = 0;
          connectionTimerRestarted = false;
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
            String _pinRaw2 = String(bleKeyboardManager.getStaticPIN());
            while (_pinRaw2.length() < 6) _pinRaw2 = "0" + _pinRaw2;
            String pinMsg = "Enter PIN: " + _pinRaw2;
            displayManager.drawBleAdvertisingPage(
                bleKeyboardManager.getDeviceName(), pinMsg, 0);
            pinPageDrawn = true;
          }

          // Таймаут аутентификации - 30 секунд
          if (currentMode == AppMode::BLE_ADVERTISING &&
              millis() - bleStartTime > 30000) {
            LOG_WARNING("Main", "Authentication timeout - disconnecting");
            bleKeyboardManager.end();
            currentMode = AppMode::PASSWORD;
            bleActionTriggered = false;
            autoSendDone = false;
            bleInitialized = false;
            pinPageDrawn = false;
            connectionTimerRestarted = false;
          }
        }
      } else {
        // Устройство не подключено - сбрасываем флаг возможности перезапуска таймера
        connectionTimerRestarted = false;
      }

      // Handle back button press
      if (readBtn1()) {
        delay(200); // Debounce
        LOG_INFO("Main", "Back button pressed - exiting BLE mode");
        bleKeyboardManager.end();
        currentMode = AppMode::PASSWORD;
        bleActionTriggered = false;
        autoSendDone = false;
        bleInitialized = false;
        bleStartTime = 0;
        lastActivityTime = millis(); // Reset timeout — user just interacted
      }
    } break;

    case AppMode::BLE_PIN_ENTRY: {
      LOG_INFO("Main", "BLE secure connection established. Checking Device BLE "
                       "PIN requirements...");

      // 🔒 Гарантируем что экран включён перед PIN вводом.
      // Накопленный таймаут мог вырубить экран в момент перехода из
      // BLE_ADVERTISING → BLE_PIN_ENTRY (race condition с isScreenOn).
      lastActivityTime = millis(); // сброс таймера — начинаем отсчёт заново
      if (!isScreenOn) {
        LOG_INFO("Main", "Screen was off at BLE_PIN_ENTRY — turning on for PIN input");
        displayManager.turnOn();
        isScreenOn = true;
        isDimmed = false;
      } else if (isDimmed) {
        displayManager.setBrightness(255);
        isDimmed = false;
      }

      bool pinOk = true;

      // Проверяем включен ли Device BLE PIN
      if (CryptoManager::getInstance().isDeviceBlePinEnabled()) {
        LOG_INFO("Main", "Device BLE PIN protection enabled, requesting PIN "
                         "for transmission...");
        lastActivityTime = millis(); // второй сброс — непосредственно перед блокирующим вызовом
        pinOk = pinManager.requestDeviceBlePinForTransmission();
      } else {
        LOG_INFO("Main", "Device BLE PIN protection disabled. Proceeding to "
                         "send confirmation");
      }

      // После возврата из блокирующего вызова — снова сбрасываем таймер,
      // чтобы экран не гас сразу при переходе в BLE_CONFIRM_SEND
      lastActivityTime = millis();

      if (pinOk) {
        LOG_INFO("Main", "BLE access granted. Proceeding to send confirmation");
        currentMode = AppMode::BLE_CONFIRM_SEND;
      } else {
        LOG_WARNING("Main",
                    "Device BLE PIN incorrect or cancelled. Returning to list");
        bleKeyboardManager.end();
        currentMode = AppMode::PASSWORD;
        bleActionTriggered = false;
        autoSendDone = false;
      }
    } break;

    case AppMode::BLE_CONFIRM_SEND: {
      static bool confirmPageDrawn = false;

      const auto& passwords = passwordManager.getAllPasswords();
      if (passwords.empty() || currentPasswordIndex >= (int)passwords.size()) {
        // Safety check
        currentMode = AppMode::PASSWORD;
        bleKeyboardManager.end();
        bleActionTriggered = false;
        autoSendTriggered = false;
        autoSendDone = false;
        confirmPageDrawn = false;
        break;
      }

      // Рисуем страницу только один раз или при принудительной перерисовке
      if (!confirmPageDrawn || previousPasswordIndex == -1) {
        String passwordName = passwords[currentPasswordIndex].name;
        String password = passwords[currentPasswordIndex].password;
        String deviceName = bleKeyboardManager.getDeviceName();
        displayManager.drawBleConfirmPage(
            passwordName, 
            password, 
            deviceName,
            passwords[currentPasswordIndex].login,
            passwords[currentPasswordIndex].getSendLogin()
        );
        confirmPageDrawn = true;
        previousPasswordIndex = currentPasswordIndex;

        // Автоотправка без нажатия кнопки
        if (currentPasswordIndex >= 0 && !autoSendDone) {
          if (passwords[currentPasswordIndex].getAutoSend()) {
            LOG_INFO("Main", "Auto-send: triggering without button press");
            autoSendTriggered = true;
          }
        }
      }

      // Wait for button press or auto-send trigger
      if (readBtn1()) { // Back button
        delay(200);
        LOG_INFO("Main", "Back button pressed. Cancelling send");
        currentMode = AppMode::PASSWORD;
        bleKeyboardManager.end();
        bleActionTriggered = false;
        autoSendTriggered = false;
        autoSendDone = false;
        confirmPageDrawn = false;
        lastActivityTime = millis(); // Reset timeout — user just interacted
      } else if (readBtn2() || autoSendTriggered) { // Send button or Auto-send
        if (!autoSendTriggered) delay(200); // Only delay for physical button
        LOG_INFO("Main", autoSendTriggered ? "Auto-sending data" : "Send button pressed. Sending data");

        displayManager.drawBleSendingPage(
            passwords[currentPasswordIndex].name,
            passwords[currentPasswordIndex].login,
            passwords[currentPasswordIndex].getSendLogin()
        );
        String password = passwords[currentPasswordIndex].getWildcard()
            ? getWildcardSessionPassword(currentPasswordIndex, passwords[currentPasswordIndex].wildcard_len)
            : passwords[currentPasswordIndex].password;
        if (passwords[currentPasswordIndex].getSendLogin() &&
            passwords[currentPasswordIndex].login.length() > 0) {
          bleKeyboardManager.sendPassword(passwords[currentPasswordIndex].login.c_str());
          if (passwords[currentPasswordIndex].nav_mode == "tab") {
            bleKeyboardManager.sendTab();
          } else {
            bleKeyboardManager.sendEnter();
          }
          delay(100);
        }
        bleKeyboardManager.sendPassword(password.c_str());
        // Wipe the local wildcard/plaintext password copy now that it
        // has been transmitted — sendPassword() has already read it.
        secureWipeString(password);
        if (passwords[currentPasswordIndex].getAutoSend()) {
          bleKeyboardManager.sendEnter();
        }
        delay(500); // Give time for the UI and BLE

        displayManager.drawBleResultPage(true); // Show success
        delay(1500);

        autoSendTriggered = false; // Сбрасываем флаг
        autoSendDone = true;       // Помечаем, что автоотправка выполнена

        // Возвращаемся к странице подтверждения для повторной отправки
        LOG_INFO("Main",
                 "Password sent successfully. Returning to confirmation page");
        previousPasswordIndex = -1; // Force redraw of confirm page
      }
    } break;

#ifdef BOARD_HAS_USB_HID
    case AppMode::USB_HID_SEND: {
      static bool usbPageDrawn = false;
      static String lastUsbStatusDrawn = "";
      
      const auto& passwords = passwordManager.getAllPasswords();
      if (passwords.empty() || currentPasswordIndex >= (int)passwords.size()) {
        currentMode = AppMode::PASSWORD;
        bleActionTriggered = false;
        autoSendDone = false;
        usbPageDrawn = false;
        displayManager.setUsbHidMode(false);
        usbHidManager.end();
        break;
      }

      if (!usbPageDrawn) {
        usbHidManager.begin();
        displayManager.setUsbHidMode(true);
        displayManager.resetUsbHidPage();
        delay(200);
        usbPageDrawn = true;
      }

      String usbStatus = usbHidManager.isConnected() ? "USB Connected" : "Waiting for USB...";
      String passwordName = passwords[currentPasswordIndex].name;
      static String pendingUsbStatus = "";
      static unsigned long pendingUsbStatusSince = 0;
      if (usbStatus != pendingUsbStatus) {
        pendingUsbStatus = usbStatus;
        pendingUsbStatusSince = millis();
      }
      if (usbStatus != lastUsbStatusDrawn && millis() - pendingUsbStatusSince >= 300) {
        displayManager.drawUsbHidPage(
            passwordName, 
            usbStatus,
            passwords[currentPasswordIndex].login,
            passwords[currentPasswordIndex].getSendLogin()
        );
        lastUsbStatusDrawn = usbStatus;
      }

      // BTN2 — отправить (правая)
      if (readBtn2()) {
        delay(50);
        if (readBtn2()) {
          String password = passwords[currentPasswordIndex].getWildcard()
              ? getWildcardSessionPassword(currentPasswordIndex, passwords[currentPasswordIndex].wildcard_len)
              : passwords[currentPasswordIndex].password;
          displayManager.drawUsbHidPage(
              passwordName, 
              "Sending...",
              passwords[currentPasswordIndex].login,
              passwords[currentPasswordIndex].getSendLogin()
          );
          if (passwords[currentPasswordIndex].getSendLogin() &&
              passwords[currentPasswordIndex].login.length() > 0) {
            usbHidManager.sendPassword(passwords[currentPasswordIndex].login.c_str());
            if (passwords[currentPasswordIndex].nav_mode == "tab") {
              usbHidManager.sendTab();
            } else {
              usbHidManager.sendEnter();
            }
            delay(100);
          }
          usbHidManager.sendPassword(password.c_str());
          // Wipe the local wildcard/plaintext password copy now that it
          // has been transmitted — sendPassword() has already read it.
          secureWipeString(password);
          if (passwords[currentPasswordIndex].getAutoSend()) {
            usbHidManager.sendEnter();
          }
          delay(500);
          lastUsbStatusDrawn = "";
        }
      }

      // BTN1 — назад (левая)
      if (readBtn1()) {
        delay(50);
        if (readBtn1()) {
          usbHidManager.end();
          displayManager.setUsbHidMode(false);
          bleActionTriggered = false;
          autoSendDone = false;
          usbPageDrawn = false;
          lastUsbStatusDrawn = "";
          pendingUsbStatus = "";
          pendingUsbStatusSince = 0;
          displayManager.resetUsbHidPage();
          previousPasswordIndex = -1;
          currentMode = AppMode::PASSWORD;
        }
      }

      break;
    }
#endif

    case AppMode::WIFI_CONFIG:
      break;
    }
  }

  // Check for scheduled restart
  if (shouldRestart) {
    LOG_INFO("Main", "Device restart requested. Restarting in 1 second...");
    delay(1000);
    secureRestart();
  }
}
