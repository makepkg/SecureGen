#ifndef APP_MODES_H
#define APP_MODES_H

enum class AppMode { 
    TOTP, 
    PASSWORD, 
    BLE_ADVERTISING, 
    BLE_PIN_ENTRY, 
    BLE_CONFIRM_SEND, 
    USB_HID_SEND,
    WIFI_CONFIG 
};

#endif // APP_MODES_H
