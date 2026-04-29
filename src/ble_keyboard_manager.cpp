#include "ble_keyboard_manager.h"
#include "log_manager.h"
#include "crypto_manager.h"
#include <esp_task_wdt.h>

#define SECURITY_LOG(msg) Serial.println("[SEC] " + String(millis()) + ": " + msg)

// HID Report Map для клавиатуры
const uint8_t BleKeyboardManager::hidReportMap[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    
    // Modifier keys
    0x05, 0x07,                    // USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    // USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    // USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x95, 0x08,                    // REPORT_COUNT (8)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    
    // Reserved byte
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x81, 0x03,                    // INPUT (Cnst,Var,Abs)
    
    // Key codes
    0x95, 0x06,                    // REPORT_COUNT (6)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x65,                    // LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    // USAGE_PAGE (Keyboard)
    0x19, 0x00,                    // USAGE_MINIMUM (Reserved)
    0x29, 0x65,                    // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    // INPUT (Data,Ary,Abs)
    
    // Output report (LEDs)
    0x95, 0x05,                    // REPORT_COUNT (5)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x05, 0x08,                    // USAGE_PAGE (LEDs)
    0x19, 0x01,                    // USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    // USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    // OUTPUT (Data,Var,Abs)
    
    // Padding
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x75, 0x03,                    // REPORT_SIZE (3)
    0x91, 0x03,                    // OUTPUT (Cnst,Var,Abs)
    
    0xc0                           // END_COLLECTION
};

BleKeyboardManager::BleKeyboardManager(const char* deviceName, const char* manufacturer, uint8_t batteryLevel) : 
    deviceConnected(false),
    secureConnected(false),
    bondingEstablished(false),
    staticPIN(123456),
    localDeviceName(deviceName),
    manufacturer(manufacturer),
    batteryLevel(batteryLevel)
{
}

BleKeyboardManager::~BleKeyboardManager() {
    end();
}

bool BleKeyboardManager::begin() {
    SECURITY_LOG("Starting secure BLE keyboard service");
    
    try {
        // Сбрасываем флаг остановки
        isShuttingDown = false;
        
        // КРИТИЧНО: Загружаем PIN из защищенного хранилища ПЕРЕД инициализацией
        loadBlePinFromCrypto();
        
        // Инициализация BLE устройства
        BLEDevice::init(localDeviceName.c_str());
        
        // КРИТИЧНО: Создаем сервер ТОЛЬКО при первой инициализации
        if (!bleInitialized || pServer == nullptr) {
            pServer = BLEDevice::createServer();
            pServer->setCallbacks(this);
        }
        
        // КРИТИЧНО: Настройка безопасности ПЕРЕД созданием сервисов
        setupSecurity();
        
        // Создание HID сервиса с защищенными характеристиками
        setupHIDService();
        
        // КРИТИЧНО: Запуск сервиса ТОЛЬКО при первой инициализации
        if (!bleInitialized) {
            pService->start();
        }
        
        // Настройка и запуск рекламы
        startAdvertising();
        
        // КРИТИЧНО: Устанавливаем флаг ТОЛЬКО в конце!
        bleInitialized = true;
        
        SECURITY_LOG("Secure BLE Keyboard started successfully");
        return true;
        
    } catch (const std::exception& e) {
        SECURITY_LOG("Failed to start BLE: " + String(e.what()));
        return false;
    }
}

void BleKeyboardManager::end() {
    SECURITY_LOG("Stopping BLE keyboard service");
    
    // 0. КРИТИЧНО: Устанавливаем флаг ДО disconnect
    isShuttingDown = true;
    
    // 1. СНАЧАЛА останавливаем advertising
    if (pAdvertising) {
        pAdvertising->stop();
        SECURITY_LOG("Advertising stopped");
    }
    
    // 2. Отключаем все активные соединения
    if (pServer && deviceConnected) {
        pServer->disconnect(pServer->getConnId());
        delay(100); // Даем время на disconnect
        SECURITY_LOG("Active connections disconnected");
    }
    
    // 3. Очищаем состояние
    deviceConnected = false;
    secureConnected = false;
    
    // 4. КРИТИЧНО: НЕ обнуляем pServer - переиспользуем!
    // pServer ОСТАЕТСЯ ДЛЯ повторной инициализации
    pService = nullptr;
    pInputCharacteristic = nullptr;
    pOutputCharacteristic = nullptr;
    pAdvertising = nullptr;
    
    // 5. КРИТИЧНО: НЕ вызываем deinit() - переиспользуем BLE стек!
    // BLEDevice::deinit(true); <- НЕ вызываем!
    
    // 6. Минимальная задержка
    delay(200);
    
    SECURITY_LOG("BLE keyboard service stopped completely");
}

void BleKeyboardManager::setupSecurity() {
    SECURITY_LOG("Configuring LE Secure Connections");
    
    // ВАЖНО: Порядок вызовов критичен!
    BLESecurity *pSecurity = new BLESecurity();
    
    // 1. СНАЧАЛА колбэки
    BLEDevice::setSecurityCallbacks(this);
    
    // 2. Статический PIN
    pSecurity->setStaticPIN(staticPIN);
    
    // 3. ПРИНУДИТЕЛЬНАЯ аутентификация - Display Only для обязательного PIN
    pSecurity->setCapability(ESP_IO_CAP_OUT);
    
    // 4. Режим аутентификации с MITM для принудительного PIN
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
    
    // 5. Инициируем bonding
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    
    SECURITY_LOG("Security configured with MANDATORY PIN: [PROTECTED]");
}

void BleKeyboardManager::setupHIDService() {
    SECURITY_LOG("Setting up HID service with encryption");
    
    // КРИТИЧНО: Условное создание в зависимости от bleInitialized
    if (!bleInitialized) {
        // Первая инициализация - создаем всё новое
        pService = pServer->createService("1812"); // HID Service UUID
        
        // Input Report (клавиши -> хост)
        pInputCharacteristic = pService->createCharacteristic(
            "2A4D",
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
        );
        pInputCharacteristic->setAccessPermissions(
            ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM
        );
        pInputCharacteristic->addDescriptor(new BLE2902());
        
        // Output Report (хост -> LED)
        pOutputCharacteristic = pService->createCharacteristic(
            "2A4E",
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
        );
        pOutputCharacteristic->setAccessPermissions(
            ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM
        );
        
        // Report Map Characteristic
        BLECharacteristic* pReportMap = pService->createCharacteristic(
            "2A4B",
            BLECharacteristic::PROPERTY_READ
        );
        pReportMap->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
        pReportMap->setValue((uint8_t*)hidReportMap, sizeof(hidReportMap));
        
        // HID Information Characteristic
        BLECharacteristic* pHidInfo = pService->createCharacteristic(
            "2A4A",
            BLECharacteristic::PROPERTY_READ
        );
        uint8_t hidInfo[] = {0x11, 0x01, 0x00, 0x03};
        pHidInfo->setValue(hidInfo, 4);
        pHidInfo->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
    } else {
        // Повторная инициализация - получаем существующее
        pService = pServer->getServiceByUUID("1812");
        if (pService) {
            pInputCharacteristic = pService->getCharacteristic("2A4D");
            pOutputCharacteristic = pService->getCharacteristic("2A4E");
        }
    }
    
    SECURITY_LOG("HID service configured with encrypted characteristics");
}

void BleKeyboardManager::startAdvertising() {
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("1812");
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    SECURITY_LOG("BLE advertising started");
}

// Колбэки безопасности
uint32_t BleKeyboardManager::onPassKeyRequest() {
    SECURITY_LOG("PassKey Request - returning static PIN");
    return staticPIN;
}

void BleKeyboardManager::onPassKeyNotify(uint32_t pass_key) {
    SECURITY_LOG("PassKey notification received");
    
    // Отображаем PIN маленьким текстом внизу экрана (убран крупный текст по центру)
    if (pDisplayManager) {
        String pinMessage = "PIN: " + String(pass_key);
        pDisplayManager->showMessage(pinMessage, 10, 110, false, 1); // Изменен размер на 1 и позиция на 110 (внизу)
        SECURITY_LOG("PIN displayed on device screen");
    } else {
        SECURITY_LOG("WARNING: DisplayManager not set, PIN not shown on screen");
    }
}

bool BleKeyboardManager::onSecurityRequest() {
    SECURITY_LOG("Security Request - accepting");
    return true;
}

bool BleKeyboardManager::onConfirmPIN(uint32_t pin) {
    SECURITY_LOG("Confirm PIN: [PROTECTED] (Expected: [PROTECTED])");
    return (pin == staticPIN);
}

void BleKeyboardManager::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
    if (cmpl.success) {
        secureConnected = true;
        
        // Проверяем статус bonding
        bool wasNewBond = !bondingEstablished;
        bondingEstablished = true;
        
        if (wasNewBond) {
            SECURITY_LOG("NEW BONDING: Keys saved to NVS");
        } else {
            SECURITY_LOG("EXISTING BONDING: Keys loaded from NVS");
        }
        
        logBondingStatus();
        SECURITY_LOG("SECURE CONNECTION ESTABLISHED!");
    } else {
        secureConnected = false;
        bondingEstablished = false;
        SECURITY_LOG("Authentication FAILED: " + String(cmpl.fail_reason));
        // Принудительно разорвать соединение
        if (pServer && deviceConnected) {
            pServer->disconnect(pServer->getConnId());
        }
    }
}

// Колбэки сервера
void BleKeyboardManager::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    
    // Проверяем существующий bonding при подключении
    esp_bd_addr_t addr;
    if (esp_ble_get_bond_device_num() > 0) {
        bondingEstablished = true;
        SECURITY_LOG("Device connected - existing bond found");
    } else {
        bondingEstablished = false;
        SECURITY_LOG("Device connected - no existing bond");
    }
    
    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Принудительно активируем notifications
    // Это необходимо для повторных подключений с bonding
    if (pInputCharacteristic) {
        BLE2902* p2902 = (BLE2902*)pInputCharacteristic->getDescriptorByUUID("2902");
        if (p2902) {
            uint8_t notificationValue[2] = {0x01, 0x00}; // Enable notifications
            p2902->setValue(notificationValue, 2);
            SECURITY_LOG("CCCD force-enabled for bonded reconnection");
        }
    }
}

void BleKeyboardManager::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    secureConnected = false;
    // bondingEstablished сохраняем для следующего подключения
    SECURITY_LOG("Device disconnected");
    
    // КРИТИЧНО: НЕ перезапускать advertising при остановке!
    if (!isShuttingDown) {
        startAdvertising(); // Перезапуск рекламы
        SECURITY_LOG("BLE advertising restarted after disconnect");
    } else {
        SECURITY_LOG("BLE shutdown in progress - NOT restarting advertising");
    }
}

void BleKeyboardManager::press(uint8_t key, uint8_t modifier) {
    // КРИТИЧЕСКАЯ ПРОВЕРКА
    if (!deviceConnected) {
        SECURITY_LOG("БЛОКИРОВКА: Устройство не подключено");
        return;
    }
    
    if (!secureConnected) {
        SECURITY_LOG("БЛОКИРОВКА: Соединение не защищено!");
        return;
    }
    
    // Проверяем состояние характеристики
    if (!pInputCharacteristic) {
        SECURITY_LOG("ОШИБКА: Input характеристика не инициализирована!");
        return;
    }
    
    // Формируем HID report
    uint8_t report[8] = {0};
    report[0] = modifier;  // Модификаторы (Shift, Ctrl, Alt, GUI)
    report[1] = 0;         // Зарезервировано
    report[2] = key;       // Код клавиши
    // report[3-7] остаются 0
    
    // Отправляем через защищенную характеристику
    pInputCharacteristic->setValue(report, 8);
    pInputCharacteristic->notify();
}

void BleKeyboardManager::release() {
    if (!deviceConnected || !secureConnected) {
        return;
    }
    
    // Отправляем пустой report (все клавиши отпущены)
    uint8_t report[8] = {0};
    pInputCharacteristic->setValue(report, 8);
    pInputCharacteristic->notify();
}

void BleKeyboardManager::print(const String& text) {
    int length = text.length();
    SECURITY_LOG("Secure transmission started (" + String(length) + " chars)");
    
    for (int i = 0; i < length; i++) {
        esp_task_wdt_reset(); // Сброс Watchdog Timer
        
        if (!deviceConnected || !secureConnected) {
            SECURITY_LOG("Connection lost at char " + String(i + 1) + "/" + String(length));
            break;
        }
        
        char c = text.charAt(i);
        uint8_t key = charToHidKey(c);
        uint8_t modifier = charToModifier(c);
        
        press(key, modifier);
        delay(50);
        release();
        delay(50);
    }
    
    SECURITY_LOG("Secure transmission completed (" + String(length) + " chars)");
}

void BleKeyboardManager::sendPassword(const char* password) {
    // КРИТИЧЕСКАЯ ПРОВЕРКА
    if (!isSecure()) {
        SECURITY_LOG("ОТКАЗ: Соединение не защищено!");
        return;
    }
    
    print(String(password));
}

uint8_t BleKeyboardManager::charToHidKey(char c) {
    // Буквы
    if (c >= 'a' && c <= 'z') return c - 'a' + 0x04;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 0x04;
    
    // Цифры (основные)
    if (c >= '1' && c <= '9') return c - '1' + 0x1E;
    if (c == '0') return 0x27;
    
    // Специальные символы с Shift + цифры (US клавиатура)
    if (c == '!') return 0x1E; // Shift + 1
    if (c == '@') return 0x1F; // Shift + 2
    if (c == '#') return 0x20; // Shift + 3
    if (c == '$') return 0x21; // Shift + 4
    if (c == '%') return 0x22; // Shift + 5
    if (c == '^') return 0x23; // Shift + 6
    if (c == '&') return 0x24; // Shift + 7
    if (c == '*') return 0x25; // Shift + 8
    if (c == '(') return 0x26; // Shift + 9
    if (c == ')') return 0x27; // Shift + 0
    
    // Управляющие символы
    if (c == ' ') return 0x2C;  // Пробел
    if (c == '\n') return 0x28; // Enter
    if (c == '\t') return 0x2B; // Tab
    
    // Знаки препинания (основные)
    if (c == '-') return 0x2D;
    if (c == '=') return 0x2E;
    if (c == '[') return 0x2F;
    if (c == ']') return 0x30;
    if (c == '\\') return 0x31;
    if (c == ';') return 0x33;
    if (c == '\'') return 0x34;
    if (c == '`') return 0x35;
    if (c == ',') return 0x36;
    if (c == '.') return 0x37;
    if (c == '/') return 0x38;
    
    // Специальные символы с Shift + знаки препинания
    if (c == '_') return 0x2D; // Shift + -
    if (c == '+') return 0x2E; // Shift + =
    if (c == '{') return 0x2F; // Shift + [
    if (c == '}') return 0x30; // Shift + ]
    if (c == '|') return 0x31; // Shift + \
    if (c == ':') return 0x33; // Shift + ;
    if (c == '"') return 0x34; // Shift + '
    if (c == '~') return 0x35; // Shift + `
    if (c == '<') return 0x36; // Shift + ,
    if (c == '>') return 0x37; // Shift + .
    if (c == '?') return 0x38; // Shift + /
    
    return 0;
}

uint8_t BleKeyboardManager::charToModifier(char c) {
    // Заглавные буквы
    if (c >= 'A' && c <= 'Z') return 0x02; // Left Shift
    
    // Специальные символы с Shift + цифры
    if (c == '!') return 0x02; // Shift + 1
    if (c == '@') return 0x02; // Shift + 2
    if (c == '#') return 0x02; // Shift + 3
    if (c == '$') return 0x02; // Shift + 4
    if (c == '%') return 0x02; // Shift + 5
    if (c == '^') return 0x02; // Shift + 6
    if (c == '&') return 0x02; // Shift + 7
    if (c == '*') return 0x02; // Shift + 8
    if (c == '(') return 0x02; // Shift + 9
    if (c == ')') return 0x02; // Shift + 0
    
    // Специальные символы с Shift + знаки препинания
    if (c == '_') return 0x02; // Shift + -
    if (c == '+') return 0x02; // Shift + =
    if (c == '{') return 0x02; // Shift + [
    if (c == '}') return 0x02; // Shift + ]
    if (c == '|') return 0x02; // Shift + \
    if (c == ':') return 0x02; // Shift + ;
    if (c == '"') return 0x02; // Shift + '
    if (c == '~') return 0x02; // Shift + `
    if (c == '<') return 0x02; // Shift + ,
    if (c == '>') return 0x02; // Shift + .
    if (c == '?') return 0x02; // Shift + /
    
    return 0;
}

void BleKeyboardManager::forceDisconnect() {
    if (pServer && deviceConnected) {
        SECURITY_LOG("Forcing BLE disconnection");
        pServer->disconnect(0);
        deviceConnected = false;
        secureConnected = false;
        bondingEstablished = false;
    }
}

void BleKeyboardManager::clearBondingKeys() {
    SECURITY_LOG("Clearing all BLE bonding keys");
    
    // First disconnect if connected
    if (deviceConnected) {
        forceDisconnect();
        delay(100); // Short delay for clean disconnection
    }
    
    // Clear bonding keys from NVS storage
    int deviceCount = esp_ble_get_bond_device_num();
    if (deviceCount > 0) {
        SECURITY_LOG("Found " + String(deviceCount) + " bonded devices to clear");
        
        // Get list of bonded devices
        esp_ble_bond_dev_t *bond_device = (esp_ble_bond_dev_t *) malloc(sizeof(esp_ble_bond_dev_t) * deviceCount);
        if (bond_device) {
            esp_ble_get_bond_device_list(&deviceCount, bond_device);
            
            // Remove each bonded device
            for (int i = 0; i < deviceCount; i++) {
                esp_ble_remove_bond_device(bond_device[i].bd_addr);
                SECURITY_LOG("Removed bonded device " + String(i + 1) + "/" + String(deviceCount));
            }
            
            free(bond_device);
            SECURITY_LOG("All BLE bonding keys cleared successfully");
        } else {
            SECURITY_LOG("ERROR: Failed to allocate memory for bonding key clearing");
        }
    } else {
        SECURITY_LOG("No bonded devices found to clear");
    }
    
    // Reset bonding state
    bondingEstablished = false;
}

const char* BleKeyboardManager::getDeviceName() {
    return localDeviceName.c_str();
}

void BleKeyboardManager::setDeviceName(const String& deviceName) {
    localDeviceName = deviceName;
    
    // Для смены имени нужен полный перезапуск BLE
    bool wasRunning = (pServer != nullptr);
    if (wasRunning) {
        // BLE запущен - нужен перезапуск для применения нового имени
        SECURITY_LOG("BLE device name changed, restarting BLE service");
        end();
        delay(100);
        begin();
        SECURITY_LOG("BLE service restarted with new name: " + deviceName);
    } else {
        // BLE не запущен - просто устанавливаем имя для будущего использования
        LOG_DEBUG("BleKeyboardManager", "BLE device name set to: " + deviceName + " (will be used when BLE starts)");
    }
}

void BleKeyboardManager::loadBlePinFromCrypto() {
    CryptoManager& crypto = CryptoManager::getInstance();
    uint32_t loadedPin = crypto.loadBlePin();
    
    if (loadedPin != staticPIN) {
        staticPIN = loadedPin;
        SECURITY_LOG("BLE PIN loaded from secure storage: [PROTECTED]");
    } else {
        SECURITY_LOG("Using default BLE PIN: [PROTECTED]");
    }
}

void BleKeyboardManager::logBondingStatus() {
    int bondedCount = esp_ble_get_bond_device_num();
    
    if (bondedCount > 0) {
        // Логируем только количество доверенных устройств без раскрытия MAC-адресов
        SECURITY_LOG("BONDING: " + String(bondedCount) + " trusted device(s) in NVS");
    } else {
        SECURITY_LOG("BONDING: No devices in NVS storage");
    }
}