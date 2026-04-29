#include "usb_hid_manager.h"

#ifdef BOARD_HAS_USB_HID

UsbHidManager::UsbHidManager() {}

bool UsbHidManager::begin() {
  if (_started) return true;
  _keyboard.begin();
  USB.begin();
  _started = true;
  delay(100);
  return true;
}

void UsbHidManager::end() {
  _started = false;
}

bool UsbHidManager::isConnected() {
  return _started;
}

void UsbHidManager::sendPassword(const char* password) {
  if (!_started) return;
  _keyboard.print(String(password));
}

uint8_t UsbHidManager::charToHidKey(char c) {
  if (c >= 'a' && c <= 'z') return c - 'a' + 0x04;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 0x04;
  if (c >= '1' && c <= '9') return c - '1' + 0x1E;
  if (c == '0') return 0x27;
  
  if (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
      c == '^' || c == '&' || c == '*' || c == '(' || c == ')')
    return charToHidKey("1234567890"[c == '!' ? 0 : c == '@' ? 1 :
                                      c == '#' ? 2 : c == '$' ? 3 : c == '%' ? 4 :
                                      c == '^' ? 5 : c == '&' ? 6 : c == '*' ? 7 :
                                      c == '(' ? 8 : 9]);
  
  if (c == ' ') return 0x2C;
  if (c == '\n') return 0x28;
  if (c == '\t') return 0x2B;
  if (c == '-' || c == '_') return 0x2D;
  if (c == '=' || c == '+') return 0x2E;
  if (c == '[' || c == '{') return 0x2F;
  if (c == ']' || c == '}') return 0x30;
  if (c == '\\' || c == '|') return 0x31;
  if (c == ';' || c == ':') return 0x33;
  if (c == '\'' || c == '"') return 0x34;
  if (c == '`' || c == '~') return 0x35;
  if (c == ',' || c == '<') return 0x36;
  if (c == '.' || c == '>') return 0x37;
  if (c == '/' || c == '?') return 0x38;
  
  return 0;
}

uint8_t UsbHidManager::charToModifier(char c) {
  if (c >= 'A' && c <= 'Z') return 0x02; // Left Shift
  if (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
      c == '^' || c == '&' || c == '*' || c == '(' || c == ')') return 0x02;
  if (c == '_' || c == '+' || c == '{' || c == '}' || c == '|' ||
      c == ':' || c == '"' || c == '~' || c == '<' || c == '>' ||
      c == '?') return 0x02;
  return 0x00;
}

#endif // BOARD_HAS_USB_HID
