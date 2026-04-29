# ESP32 T-Display TOTP — Documentation

## Structure

### security/
- `SECURITY_OVERVIEW.md` — Public-facing security summary. Cryptographic standards, known limitations, threat model overview.
- `security-model.md` — Developer/auditor reference. Full 8-layer architecture, key hierarchy, threat model tables, new endpoint checklist.
- `pbkdf2-security.md` — PBKDF2 parameters, iteration counts, hardware constraints, compatibility notes for decrypt_export.html.

### development/
- `ENDPOINTS.md` — Complete API reference. All endpoints with auth requirements, request/response formats, security notation.
- `system_design.md` — Internal architecture. Boot sequence, PIN design, shutdown/deep sleep, data persistence table, error handling.
- `LOGGING_SYSTEM.md` — Log levels, LOG_* macros, Serial output format, filtering.
- `url-obfuscation.md` — URL obfuscation layer internals, epoch rotation, endpoint registration checklist.
- `multi-board.md` — Multi-board development rules. Three types of platform differences, scaling checklist, golden rule.

#### development/boards/
Hardware reference for supported targets. Each file covers: MCU, display, pinout, USB/flashing specifics, known quirks.

- `t-display.md` — LilyGo T-Display ESP32 (1.14", Classic ESP32, SPI display, CH340 USB).
- `t-display-s3.md` — LilyGo T-Display-S3 (1.9", ESP32-S3, 8-bit parallel display, native USB, USB HID capable).

### user/
- `GUIDE.html` — Device operation manual.
- `GUIDE.ru.html` — Device operation manual (Russian).
- `MODES.md` — Network and display modes, feature matrix, mode selection at startup.
- `decrypt-export-guide.md` — Offline export decryption tool guide.
- `rtc_mastering.md` — DS3231 RTC module connection guide. Wiring diagrams, pin assignments (ESP32/S3), enabling RTC in firmware, troubleshooting.

---

## Supported Boards

| Build target | Board | USB HID | PSRAM | Display |
|---|---|---|---|---|
| `lilygo-t-display` | T-Display ESP32 | ❌ | ❌ | 1.14" SPI |
| `lilygo-t-display-s3` | T-Display-S3 | ✅ | 8 MB | 1.9" Parallel |

Board-specific constants → `include/boards/`  
Build flags → `platformio.ini`  
Feature guards → `#ifdef BOARD_HAS_USB_HID`

---

Last updated: April 2026