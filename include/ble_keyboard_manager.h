#ifndef BLE_KEYBOARD_MANAGER_H
#define BLE_KEYBOARD_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <BLE2902.h>
#include "crypto_manager.h"
#include "display_manager.h"

class BleKeyboardManager : public BLEServerCallbacks, public BLESecurityCallbacks {
private:
    BLEServer* pServer = nullptr;
    BLEService* pService = nullptr;
    BLECharacteristic* pInputCharacteristic = nullptr;
    BLECharacteristic* pOutputCharacteristic = nullptr;
    BLEAdvertising* pAdvertising = nullptr;
    
    bool deviceConnected = false;
    bool secureConnected = false;
    bool bondingEstablished = false;
    bool bleInitialized = false; // Флаг первой инициализации
    bool isShuttingDown = false; // Флаг остановки - не перезапускать advertising
    uint32_t staticPIN = 123456;
    String localDeviceName;
    String manufacturer;
    uint8_t batteryLevel;
    DisplayManager* pDisplayManager = nullptr;
    
    // HID Report Map для клавиатуры
    static const uint8_t hidReportMap[];
    
    // Колбэки безопасности
    uint32_t onPassKeyRequest() override;
    void onPassKeyNotify(uint32_t pass_key) override;
    bool onSecurityRequest() override;
    bool onConfirmPIN(uint32_t pin) override;
    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override;
    
    // Bonding логирование
    void logBondingStatus();
    
    // Колбэки сервера
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    
    // Внутренние методы
    void setupSecurity();
    void setupHIDService();
    void startAdvertising();
    
    // Маппинг символов в HID коды
    uint8_t charToHidKey(char c);
    uint8_t charToModifier(char c);
    
public:
    BleKeyboardManager(const char* deviceName, const char* manufacturer, uint8_t batteryLevel);
    void setDisplayManager(DisplayManager* displayManager) { pDisplayManager = displayManager; }
    ~BleKeyboardManager();
    
    bool begin();
    void end();
    void press(uint8_t key, uint8_t modifier = 0);
    void release();
    void print(const String& text);
    void sendPassword(const char* password);
    void sendEnter();
    void sendTab();
    
    // Проверки состояния
    bool isConnected() const { return deviceConnected; }
    bool isSecure() const { return secureConnected; }
    
    // Настройки безопасности
    void setStaticPIN(uint32_t pin) { staticPIN = pin; }
    uint32_t getStaticPIN() const { return staticPIN; }
    void loadBlePinFromCrypto();
    void forceDisconnect();
    void clearBondingKeys(); // Clear all BLE bonding keys
    
    // Совместимость с существующим кодом
    const char* getDeviceName();
    void setDeviceName(const String& deviceName);
};

#endif // BLE_KEYBOARD_MANAGER_H