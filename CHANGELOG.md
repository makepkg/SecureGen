# Changelog

## [2.2.0] — April 2026

### Hardware Support
- **T-Display-S3 support** — added full support for ESP32-S3 board with 1.9" parallel display (170×320), 8MB PSRAM, and native USB-OTG
- **USB HID keyboard (S3 only)** — type passwords via native USB connection without BLE pairing; configurable in web cabinet Settings
- **Password security badges** — visual indicators on password screen: strength (1-3 locks), duplicate detection (DUP), PIN-only passwords, name-in-password warnings
- Better AES encryption performance on S3 hardware

### Web Interface
- **USB HID configuration** — enable/disable USB HID keyboard mode from web cabinet (S3 only)
- **Board switcher on website** — documentation site now features interactive comparison between ESP32 and S3 boards

### Bug Fixes & Stability
- Fixed memory leaks in web server handlers
- Fixed AsyncWebServer double-send crashes in body handlers
- Fixed display initialization race conditions on S3

### Documentation
- Added [Multi-Board Support](docs/development/multi-board.md) — porting guide for ESP32/S3 boards
- Updated README with hardware comparison and USB HID features
- Expanded roadmap with security enhancements and UX improvements

---

## [2.1.0] — March 2026

### Web Interface
- **Multilingual interface** — web cabinet now supports English, Russian, German, Chinese (Simplified), and Spanish; language switcher in the widget bar persists selection across sessions
- **Battery widget** — live battery level and charging indicator in the web cabinet widget bar
- **WiFi credentials management** — configure WiFi SSID and password directly from the web cabinet without AP mode
- **Boot mode setting** — configure default startup mode (WiFi / AP / Offline) from Settings tab
- Removed obsolete "instructions" page from web server

### Hardware & Stability
- **DS3231 RTC module support** — accurate offline timekeeping; enables TOTP in AP and Offline modes without WiFi or NTP
- **Light sleep remaster** — fixed crash-on-wake on battery power caused by GPIO0 hardware interaction; pseudo-sleep now uses 40 MHz CPU + display suspend instead of `esp_light_sleep_start()`
- **Auto lock** — new deep sleep option with RAM wipe after configurable inactivity timeout
- **Battery charging animation** — animated charging indicator on device screen when connected to power
- Fixed broken sign-in on main screen

---

## [2.0.0] — March 2026

### Security
- **AES-GCM transport encryption** — replaced XOR with AES-256-GCM for all web communications
- **PIN-encrypted device key** — master key file is now encrypted with PIN + salt via PBKDF2-HMAC-SHA256 (AES-256-CBC)
- **PBKDF2 iterations increased** to 25,000 (PIN unlock, login, export)
- **Persistent PIN lockout** — failed attempt counter survives reboots; device locks permanently after 5 total attempts
- **Secure memory wipe** before deep sleep — device key, TOTP secrets, passwords, and session keys zeroed from RAM

### Encryption & Key Management
- Reworked encryption and decryption system for device key, passwords, and TOTP secrets
- Added `decrypt_export.html` — offline HTML tool for decrypting exports, editing keys and passwords, and creating key files without the device

### TOTP / HOTP
- **HOTP support** — counter-based codes work in Offline and AP modes, independent of internet or time sync
- **Extended algorithm support** — SHA1 / SHA256 / SHA512, 6 and 8 digit codes, configurable period (30s / 60s)
- **QR code import** — add TOTP keys by scanning a QR code (camera or file upload)
- **QR code export** — display any TOTP key as QR code on the device screen and in the web interface
- Hold both buttons on HOTP screen to force-refresh the current code

### Web Interface & API
- Fixed broken import/export system
- Reduced number of requests from ESP32 to web server — improved performance
- Added password generation support in web cabinet

### Hardware & Stability
- Fixed crashes on battery power when pressing buttons
- Added QR code for WiFi connection in AP mode — scan to connect instantly
- Captive portal support in AP mode and WiFi setup flow
- Hold both buttons on PIN screen → deep sleep (shutdown)

### Logging
- Debug logging disabled in production builds
- Runtime log level configurable without reflashing — see [Logging System](docs/development/LOGGING_SYSTEM.md)

---

## Roadmap

See [README → Roadmap](README.md#-roadmap) for planned features.