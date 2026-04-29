#pragma once

#ifdef BOARD_HAS_USB_HID

#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

class UsbHidManager {
public:
  UsbHidManager();
  bool begin();
  void end();
  bool isConnected();
  void sendPassword(const char* password);

private:
  USBHIDKeyboard _keyboard;
  bool _started = false;
  uint8_t charToHidKey(char c);
  uint8_t charToModifier(char c);
};

#endif // BOARD_HAS_USB_HID
