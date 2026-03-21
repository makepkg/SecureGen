#include "pin_manager.h"
#include <FS.h>
#include "config.h"
#include "crypto_manager.h"
#include "log_manager.h"
#include <ArduinoJson.h>
#include "LittleFS.h"
#include <esp_task_wdt.h>

// Forward declaration for secureShutdown from main.cpp
extern void secureShutdown();

PinManager::PinManager(DisplayManager& display) : displayManager(display) {
    // Конструктор пуст
}

void PinManager::begin() {
    LOG_INFO("PinManager", "Initializing...");
    loadPinConfig();
    LOG_INFO("PinManager", "Initialized successfully");
}

void PinManager::loadPinConfig() {
    LOG_DEBUG("PinManager", "Loading PIN configuration");
    
    // Обфусцированное хранилище параметров UI
    if (LittleFS.exists("/.sys_ui_prefs")) {
        fs::File prefFile = LittleFS.open("/.sys_ui_prefs", "r");
        if (prefFile) {
            String prefData = prefFile.readStringUntil('\n');
            prefFile.close();
            int uiParam = prefData.toInt();
            if (uiParam >= 4 && uiParam <= MAX_PIN_LENGTH) {
                currentPinLength = uiParam;
                LOG_INFO("PinManager", "UI preferences loaded");
            } else {
                LOG_WARNING("PinManager", "Invalid UI parameter, using default");
                currentPinLength = DEFAULT_PIN_LENGTH;
            }
        }
    } else {
        LOG_DEBUG("PinManager", "UI preferences not found, using default");
        currentPinLength = DEFAULT_PIN_LENGTH;
    }
    
    // Загружаем остальные настройки из зашифрованного файла (только если device key инициализирован)
    if (LittleFS.exists(PIN_FILE) && CryptoManager::getInstance().isDeviceKeyInitialized()) {
        fs::File configFile = LittleFS.open(PIN_FILE, "r");
        if (configFile) {
            String encryptedContent = configFile.readString();
            configFile.close();
            
            String decryptedContent = CryptoManager::getInstance().decrypt(encryptedContent);
            if (!decryptedContent.isEmpty()) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, decryptedContent);
                if (error == DeserializationError::Ok) {
                    enabledForBle = doc["enabledForBle"] | false;
                    LOG_INFO("PinManager", "BLE PIN: " + String(enabledForBle ? "enabled" : "disabled"));
                }
            }
        }
    }
}

void PinManager::savePinConfig() {
    LOG_DEBUG("PinManager", "Saving PIN configuration");
    
    // Обфусцированное хранилище параметров UI
    fs::File prefFile = LittleFS.open("/.sys_ui_prefs", "w");
    if (prefFile) {
        prefFile.println(currentPinLength);
        prefFile.close();
        LOG_INFO("PinManager", "UI preferences saved");
    } else {
        LOG_ERROR("PinManager", "Failed to save UI preferences");
    }
    
    // Сохраняем остальные настройки в зашифрованный файл (только если device key инициализирован)
    if (!CryptoManager::getInstance().isDeviceKeyInitialized()) {
        LOG_DEBUG("PinManager", "Device key not initialized, skipping encrypted config save");
        return;
    }
    
    JsonDocument doc;
    doc["enabledForBle"] = enabledForBle;
    doc["version"] = 2; // Новая версия без хранения PIN хеша

    String content;
    size_t jsonSize = serializeJson(doc, content);
    if (jsonSize == 0) {
        LOG_ERROR("PinManager", "Failed to serialize PIN config to JSON");
        return;
    }

    String encryptedContent = CryptoManager::getInstance().encrypt(content);
    if (encryptedContent.isEmpty()) {
        LOG_ERROR("PinManager", "Failed to encrypt PIN config");
        return;
    }

    fs::File configFile = LittleFS.open(PIN_FILE, "w");
    if (configFile) {
        size_t bytesWritten = configFile.print(encryptedContent);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("PinManager", "PIN config saved successfully");
        } else {
            LOG_ERROR("PinManager", "Failed to write PIN config data");
        }
    } else {
        LOG_ERROR("PinManager", "Failed to open PIN config file for writing");
    }
}

void PinManager::drawPinScreen(const String& title, bool isConfirmScreen) {
    TFT_eSPI* tft = displayManager.getTft();
    tft->fillScreen(TFT_BLACK);
    tft->setTextDatum(MC_DATUM);

    int centerX = tft->width() / 2;

    // Рисуем заголовок
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(title, centerX, 25);
    
    // Add hint depending on screen type
    tft->setTextSize(1);
    if (isConfirmScreen) {
        tft->drawString("Hold both to go back", centerX, tft->height() - 10);
    } else {
        tft->drawString("Hold both to shutdown", centerX, tft->height() - 10);
    }
}

void PinManager::updatePinScreen(int currentPosition, int currentDigit, const String& enteredPin) {
    TFT_eSPI* tft = displayManager.getTft();
    int centerX = tft->width() / 2;

    // Обновляем только маску PIN-кода
    String pinMask = "";
    for (int i = 0; i < enteredPin.length(); i++) pinMask += "*";
    for (int i = 0; i < (currentPinLength - enteredPin.length()); i++) pinMask += ".";
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextSize(3);
    tft->fillRect(0, 50, tft->width(), 24, TFT_BLACK);
    tft->drawString(pinMask, centerX, 60);

    // Обновляем только селектор цифр
    String selector = "< " + String(currentDigit) + " >";
    tft->setTextSize(2);
    tft->fillRect(0, 85, tft->width(), 16, TFT_BLACK);
    tft->drawString(selector, centerX, 95);
}

String PinManager::requestPinInput(const String& title, bool isConfirmScreen) {
    LOG_INFO("PinManager", "PIN entry requested: " + title);

    String enteredPin = "";
    int currentDigit = 0;
    unsigned long lastButtonPress = 0;
    const int debounce = 150;
    
    displayManager.setBrightness(128); // 50% для экономии батареи
    
    drawPinScreen(title, isConfirmScreen);
    updatePinScreen(enteredPin.length(), currentDigit, enteredPin);

    while (true) {
        esp_task_wdt_reset();
        
        // Проверка отмены (обе кнопки) - переход в deep sleep
        if (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
            unsigned long pressStartTime = millis();
            while(digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
                esp_task_wdt_reset();
                if (millis() - pressStartTime > 1000) {
                    // ✅ ВЫХОДИМ ИЗ ЦИКЛА ПРЕЖДЕ ЧЕМ ДЕЛАТЬ DEEP SLEEP
                    // Ждем отпускания кнопок как в обычных нажатиях
                    while(digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW) {
                        esp_task_wdt_reset();
                        delay(20);
                    }
                    delay(30);
                    
                    if (isConfirmScreen) {
                        LOG_INFO("PinManager", "PIN confirmation cancelled - going back");
                        displayManager.setBrightness(255);
                        displayManager.init();
                        displayManager.showMessage("Going back...", 10, 50, false, 2);
                        delay(800);
                        return String("__BACK__");
                    } else {
                        LOG_INFO("PinManager", "PIN entry cancelled - entering deep sleep");
                        displayManager.setBrightness(255);
                        displayManager.init();
                        displayManager.showMessage("Shutting down...", 10, 50, false, 2);
                        delay(1000);
                        displayManager.turnOff();
                        
                        // 🔧 УСИЛЕННАЯ ЗАЩИТА ОТ СБРОСА ПРИ РАБОТЕ ОТ БАТАРЕИ
                        // 1. Отключаем watchdog чтобы он не сбросил устройство
                        esp_task_wdt_delete(NULL);
                        // 2. Даем время на стабилизацию напряжения
                        delay(100);
                        // 3. Уходим в deep sleep
                        secureShutdown();
                        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
                        esp_deep_sleep_start(); // Уходим в deep sleep до нажатия RST
                    }
                }
            }
        }

        // Кнопка 1 - переключение цифры
        if (digitalRead(BUTTON_1) == LOW && (millis() - lastButtonPress > debounce)) {
            lastButtonPress = millis();
            currentDigit = (currentDigit + 1) % 10;
            updatePinScreen(enteredPin.length(), currentDigit, enteredPin);
            
            unsigned long buttonHoldStart = millis();
            while(digitalRead(BUTTON_1) == LOW) {
                esp_task_wdt_reset();
                delay(20);
                if (millis() - buttonHoldStart > 300) break;
            }
            delay(30);
            esp_task_wdt_reset();
        }

        // Кнопка 2 - подтверждение цифры
        if (digitalRead(BUTTON_2) == LOW && (millis() - lastButtonPress > debounce)) {
            lastButtonPress = millis();
            enteredPin += String(currentDigit);
            currentDigit = 0;
            
            updatePinScreen(enteredPin.length(), currentDigit, enteredPin);
            
            unsigned long buttonHoldStart = millis();
            while(digitalRead(BUTTON_2) == LOW) {
                esp_task_wdt_reset();
                delay(20);
                if (millis() - buttonHoldStart > 300) break;
            }
            delay(30);
            esp_task_wdt_reset();

            if (enteredPin.length() >= currentPinLength) {
                displayManager.setBrightness(255);
                return enteredPin; // Возвращаем введенный PIN
            }
        }
        delay(50);
    }
}

int PinManager::requestPinLengthSelection() {
    displayManager.setBrightness(128);
    
    TFT_eSPI* tft = displayManager.getTft();
    tft->fillScreen(TFT_BLACK);
    tft->setTextDatum(MC_DATUM);
    int centerX = tft->width() / 2;
    
    // Title - same style as PIN input
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("PIN Length", centerX, 25);
    
    // Bottom hint - same as PIN input
    tft->setTextSize(1);
    tft->drawString("Hold both to shutdown", centerX, tft->height() - 10);
    
    int selectedLength = currentPinLength; // Start from current value
    
    // Draw initial selector - same style as digit selector in PIN input
    auto drawSelector = [&]() {
        String selector = "< " + String(selectedLength) + " >";
        tft->setTextSize(2);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->fillRect(0, 73, tft->width(), 16, TFT_BLACK);
        tft->drawString(selector, centerX, 83);
    };
    
    drawSelector();
    
    unsigned long lastButtonPress = 0;
    const unsigned long debounce = 200;
    
    while (true) {
        esp_task_wdt_reset();
        
        // Both buttons held - cancel
        if (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
            unsigned long pressStart = millis();
            while(digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
                esp_task_wdt_reset();
                if (millis() - pressStart > 1000) {
                    displayManager.setBrightness(255);
                    return currentPinLength; // Return unchanged
                }
            }
        }
        
        // Button 1: Change length (4-10)
        if (digitalRead(BUTTON_1) == LOW && (millis() - lastButtonPress > debounce)) {
            lastButtonPress = millis();
            selectedLength = (selectedLength >= 10) ? 4 : selectedLength + 1;
            drawSelector();
            
            unsigned long holdStart = millis();
            while(digitalRead(BUTTON_1) == LOW) {
                esp_task_wdt_reset();
                delay(20);
                if (millis() - holdStart > 300) break;
            }
            delay(30);
            esp_task_wdt_reset();
        }
        
        // Button 2: Confirm
        if (digitalRead(BUTTON_2) == LOW && (millis() - lastButtonPress > debounce)) {
            lastButtonPress = millis();
            
            unsigned long holdStart = millis();
            while(digitalRead(BUTTON_2) == LOW) {
                esp_task_wdt_reset();
                delay(20);
                if (millis() - holdStart > 300) break;
            }
            delay(30);
            esp_task_wdt_reset();
            
            displayManager.setBrightness(255);
            return selectedLength;
        }
        
        delay(50);
    }
}

bool PinManager::requestNewPinSetup() {
    LOG_INFO("PinManager", "First boot: requesting new PIN setup");
    
    // Убран экран "First Boot!" - он показывается в main.cpp ДО выбора длины
    
    // 1. Запрашиваем новый PIN
    String newPin = requestPinInput("Create New PIN", false);
    if (newPin.isEmpty()) {
        LOG_WARNING("PinManager", "PIN setup cancelled");
        return false;
    }
    
    // 2. Запрашиваем подтверждение (с возможностью вернуться назад)
    while (true) {
        String confirmPin = requestPinInput("Confirm PIN", true);
        
        if (confirmPin == "__BACK__") {
            LOG_INFO("PinManager", "User went back from confirmation to re-enter PIN");
            newPin = requestPinInput("Create New PIN", false);
            if (newPin.isEmpty()) {
                LOG_WARNING("PinManager", "PIN setup cancelled");
                return false;
            }
            continue;
        }
        
        if (confirmPin.isEmpty()) {
            LOG_WARNING("PinManager", "PIN confirmation cancelled");
            return false;
        }
        
        // 3. Проверяем совпадение
        if (newPin != confirmPin) {
            LOG_ERROR("PinManager", "PIN mismatch!");
            displayManager.init();
            displayManager.showMessage("PIN Mismatch!", 10, 30, true, 2);
            displayManager.showMessage("Try again", 10, 60, false, 2);
            delay(2000);
            return false;
        }
        
        break;
    }
    
    // 4. Создаем зашифрованный device key
    if (!CryptoManager::getInstance().createEncryptedDeviceKey(newPin)) {
        LOG_ERROR("PinManager", "Failed to create encrypted device key");
        displayManager.init();
        displayManager.showMessage("ERROR!", 10, 30, true, 2);
        displayManager.showMessage("Key creation failed", 10, 60, false, 1);
        delay(2000);
        return false;
    }
    
    // 5. Сохраняем конфигурацию PIN
    savePinConfig();
    
    LOG_INFO("PinManager", "Device PIN created successfully");
    displayManager.init();
    displayManager.showMessage("PIN Created!", 10, 30, false, 2);
    displayManager.showMessage("Device Secured", 10, 60, false, 2);
    delay(2000);
    
    return true;
}

bool PinManager::requestDevicePin() {
    LOG_INFO("PinManager", "Requesting device PIN for unlock");
    
    // ✅ Загружаем конфиг PIN (длину) перед запросом
    loadPinConfig();
    LOG_INFO("PinManager", "Loaded PIN config: length=" + String(currentPinLength));
    
    // Запрашиваем PIN
    String enteredPin = requestPinInput("Enter Device PIN");
    unsigned long waitStart = millis();
    while(digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW) {
        esp_task_wdt_reset();
        delay(20);
        if (millis() - waitStart > 5000) break;
    }
    delay(50);
    if (enteredPin.isEmpty()) {
        LOG_WARNING("PinManager", "PIN entry cancelled");
        return false;
    }
    
    // Пытаемся разблокировать device key
    TFT_eSPI* tft = displayManager.getTft();
    int centerX = tft->width() / 2;
    tft->setTextDatum(MC_DATUM);
    
    if (CryptoManager::getInstance().unlockDeviceKeyWithPin(enteredPin)) {
        LOG_INFO("PinManager", "Device unlocked successfully");
        tft->fillScreen(TFT_BLACK);
        tft->setTextSize(3);
        tft->setTextColor(TFT_GREEN);
        tft->drawString("PIN OK", centerX, 67);
        delay(1000);
        return true;
    } else {
        LOG_WARNING("PinManager", "Wrong PIN entered");
        tft->fillScreen(TFT_BLACK);
        tft->setTextSize(2);
        tft->setTextColor(TFT_RED);
        tft->drawString("WRONG PIN!", centerX, 67);
        delay(2000);
        return false;
    }
}

bool PinManager::isDevicePinRequired() {
    // Device PIN требуется если device.key зашифрован
    return CryptoManager::getInstance().isDeviceKeyEncrypted();
}

// --- BLE PIN methods ---
void PinManager::setPinEnabledForBle(bool enabled) {
    enabledForBle = enabled;
    LOG_INFO("PinManager", "BLE PIN " + String(enabled ? "enabled" : "disabled"));
}

bool PinManager::isPinEnabledForBle() {
    return enabledForBle;
}

bool PinManager::requestDeviceBlePinForTransmission() {
    LOG_INFO("PinManager", "Requesting Device BLE PIN for password transmission");
    
    // Запрашиваем PIN (всегда 6 цифр для Device BLE PIN)
    String enteredPin = requestPinInput("Enter Device BLE PIN");
    if (enteredPin.isEmpty()) {
        LOG_WARNING("PinManager", "Device BLE PIN entry cancelled");
        return false;
    }
    
    // Проверяем PIN через CryptoManager
    TFT_eSPI* tft = displayManager.getTft();
    int centerX = tft->width() / 2;
    tft->setTextDatum(MC_DATUM);
    
    if (CryptoManager::getInstance().verifyDeviceBlePin(enteredPin)) {
        LOG_INFO("PinManager", "Device BLE PIN verified successfully");
        tft->fillScreen(TFT_BLACK);
        tft->setTextSize(3);
        tft->setTextColor(TFT_GREEN);
        tft->drawString("PIN OK", centerX, 67);
        delay(1000);
        return true;
    } else {
        LOG_WARNING("PinManager", "Wrong Device BLE PIN entered");
        tft->fillScreen(TFT_BLACK);
        tft->setTextSize(2);
        tft->setTextColor(TFT_RED);
        tft->drawString("WRONG PIN!", centerX, 67);
        delay(2000);
        return false;
    }
}

// --- Configuration ---
int PinManager::getPinLength() {
    return currentPinLength;
}

void PinManager::setPinLength(int newLength) {
    if (newLength >= 4 && newLength <= MAX_PIN_LENGTH) {
        currentPinLength = newLength;
        LOG_INFO("PinManager", "PIN length set to: " + String(newLength));
    } else {
        LOG_WARNING("PinManager", "Invalid PIN length: " + String(newLength));
    }
}

void PinManager::saveConfig() {
    savePinConfig();
}

// --- Legacy/deprecated methods ---
bool PinManager::isPinEnabled() {
    return isDevicePinRequired();
}

void PinManager::setEnabled(bool enabled) {
    LOG_WARNING("PinManager", "setEnabled() is deprecated - PIN is always required when device.key is encrypted");
}

bool PinManager::isPinSet() {
    return isDevicePinRequired();
}

void PinManager::setPin(const String& newPin) {
    LOG_WARNING("PinManager", "setPin() is deprecated - use CryptoManager::changePinEncryption()");
}

bool PinManager::isPinEnabledForDevice() {
    return isDevicePinRequired();
}

void PinManager::setPinEnabledForDevice(bool enabled) {
    LOG_WARNING("PinManager", "setPinEnabledForDevice() is deprecated");
}


