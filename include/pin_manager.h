#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include "display_manager.h"

#define DEFAULT_PIN_LENGTH 6
#define MAX_PIN_LENGTH 10

class PinManager {
public:
    PinManager(DisplayManager& display);
    void begin();
    
    // --- NEW: First boot PIN setup ---
    // Запрашивает выбор длины PIN при первой загрузке
    int requestPinLengthSelection();
    // Запрашивает создание нового PIN при первой загрузке
    bool requestNewPinSetup();
    
    // --- Device PIN control ---
    bool requestDevicePin(); // Запрашивает PIN для разблокировки устройства
    bool isDevicePinRequired(); // Проверяет нужен ли PIN для загрузки
    
    // --- BLE PIN control (legacy, для обратной совместимости) ---
    bool isPinEnabledForBle();
    void setPinEnabledForBle(bool enabled);
    bool requestDeviceBlePinForTransmission(); // Запрашивает Device BLE PIN перед отправкой пароля
    
    // --- Configuration ---
    int getPinLength();
    void setPinLength(int newLength);
    void saveConfig();
    void loadPinConfig();
    
    // --- Legacy methods (deprecated, для совместимости с web_server) ---
    bool isPinEnabled(); // Returns true if device PIN is required
    void setEnabled(bool enabled); // Deprecated
    bool isPinSet(); // Deprecated - всегда true если device.key зашифрован
    void setPin(const String& newPin); // Deprecated - используйте changePinEncryption
    bool isPinEnabledForDevice(); // Deprecated
    void setPinEnabledForDevice(bool enabled); // Deprecated
    
    // --- Public PIN input method (used by main.cpp for device confirmation) ---
    String requestPinInput(const String& title, bool isConfirmScreen = false); // Универсальный ввод PIN

private:
    DisplayManager& displayManager;
    int currentPinLength = DEFAULT_PIN_LENGTH;
    bool enabledForBle = false; // Только для BLE PIN

    void savePinConfig();
    void drawPinScreen(const String& title, bool isConfirmScreen = false);
    void updatePinScreen(int currentPosition, int currentDigit, const String& enteredPin);
};

#endif // PIN_MANAGER_H
