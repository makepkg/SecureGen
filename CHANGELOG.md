# Changelog

## [2.4.1] — August 2026
### Added
- **Send Login Before Password** — new per-password option in Password Manager; when enabled, device types the login username/email first, then presses Tab or Enter (configurable), waits a specified delay (default 300 ms), and finally types the password. Useful for automated login to forms requiring both username and password fields. Configurable per entry in web cabinet: login field, navigation mode (Tab/Enter), and delay before password. LOGIN ⏎ or LOGIN ⇥ badge shown on device display and in web cabinet depending on navigation mode.
- **Wildcard Passwords** — randomly generated passwords (3-50 chars, default 16) that never leave the device. Generated on-demand per boot session with cached reuse; ideal for accounts where only HID typing is needed, not clipboard access. Singleton constraint enforced (max 1 wildcard entry). Wildcard entries cannot be copied to clipboard or exported (UI guards in web cabinet and offline decrypt tool). Session cache zeroed using `String::setCharAt()` at shutdown, mode transitions, and post-transmission. RND badge (magenta) shown on device display.
- **Secure Import/Export Mode** — TOTP keys and passwords are now exported/imported exclusively through an isolated, closed-AP session (reboot → PIN unlock → freshly generated, one-time WPA2 access point), replacing the old always-on 5-minute API-enable toggle. Deliberately outside the ordinary 8-layer web cabinet stack — smaller attack surface for a sensitive bulk-data operation, bounded by a 10-minute rolling timeout and a 30-minute hard cap. Import path validates the decrypted file's schema (TOTP keys vs. passwords) before writing, rejecting a mismatched file type with an explicit error instead of silently corrupting the store. Mirrors whichever Hidden Space is active — tagged to the requesting space so the mode only triggers for the space that requested it.

### Changed
- **BREAKING:** Removed the legacy 5-minute "enable import/export" toggle and all six of its endpoints (`/api/enable_import_export`, `/api/import_export_status`, `/api/export`, `/api/import`, `/api/passwords/export`, `/api/passwords/import`) along with their tunnel/obfuscated variants. There is no replacement at these paths — all import/export now goes exclusively through Secure Import/Export Mode (see `### Added` above and `docs/development/ENDPOINTS.md`).
- **Password flags migration** — `auto_send` flag migrated from standalone `bool` to bitmask (`uint16_t flags` in `PasswordEntry` struct). Existing entries are automatically migrated on first boot. No user action required.
- **Heap pressure reduction** — `KeyManager::getAllKeys()` and `PasswordManager::getAllPasswords()` now return `const std::vector<T>&` instead of by-value copies, eliminating repeated full-vector allocations (previously up to ~8-20 KB per call, called as often as every 250ms from the main loop TOTP display). Sorting by `order` moved from the getters into `loadKeys()`/`loadPasswords()`, `reorderKeys()`/`reorderPasswords()`, and `replaceAllKeys()`/`replaceAllPasswords()` (import), since the getters no longer mutate state.
- **Encrypted transport encoding** — the secure request/response envelope (`data`/`iv`/`tag` fields) now uses base64 instead of hex encoding, reducing payload size by ~30%. This is a protocol-breaking change: server and client must be updated together; existing sessions are invalidated on upgrade.
- **Removed legacy `firmware/` directory** — firmware binaries have been served from `website/public/firmware/` (published via GitHub Pages) since the previous release; the old root-level `firmware/` copies were stale leftovers with no remaining references in the build workflow, web flasher, or documentation.

### Fixed
- **TOTP/password reorder consistency** — fixed a latent bug where `reorderKeys()`/`reorderPasswords()` and key/password import did not re-sort the in-memory list after changing entry order, relying on the (now removed) sort-on-every-read behavior of the getters. Reordering and import now sort explicitly, so the new order is reflected immediately without requiring a reboot.
- **Password export buffer overflow** — increased `decryptWithPassword()` JSON buffer from 2KB to 16KB to support exporting 50+ password entries. Previously, exports exceeding ~15-20 passwords would fail silently, returning `{"ciphertext": null}` or empty responses due to insufficient buffer space for base64-encoded ciphertext. The new limit supports up to ~100 password entries safely.
- **Hidden Space files orphaned on removal** — `wipeHiddenSpace()` derived the HMAC paths of Space B's private files (`keys`, `passwords`, `wifi`, `session`, etc.) using whichever device key happened to be loaded, which was always Space A's key at every real call site. Since those paths are only correct when derived from Space B's own key, the files were never actually found or deleted — despite logs and the web response claiming a full wipe. Replaced with `removeHiddenSpaceWithPin(spaceBPin)`, which uses the existing non-destructive `tryDecryptSlotDry()` to obtain Space B's key into a local buffer without disturbing the caller's active Space A session, then addresses and deletes the files correctly. The sentinel file (intentionally addressed via Space A's key by design) is unaffected by this fix.
- **Password screen not redrawn after QR code dismissal** — displaying a TOTP key's QR code (`/api/show_qr`) while the device was in Password mode left the screen blank after the 30-second auto-hide timer expired, or after manual dismissal via button press. TOTP mode already had an internal redraw flag consumed on its next code-refresh cycle; Password mode had no equivalent, so its own change-detection condition (index/revision unchanged) never fired. Added a mirrored, self-clearing redraw flag consumed on the next Password-mode loop iteration.
- **False RTC "drifted" status on timezone mismatch** — `GET /api/rtc` serialized the RTC's UTC epoch through `localtime_r()` before sending it, embedding the device's configured timezone offset into a timestamp string with no UTC indicator. The browser then parsed that string in its own local timezone, so the client-side drift calculation silently combined two independent timezone offsets whenever the device timezone was unset or didn't match the browser's. Switched to `gmtime_r()` with a trailing `Z` suffix across all three response paths (standard, tunneled, obfuscated), so the client always receives and parses a true UTC timestamp.
- **Duress PIN left as an orphan after disabling startup PIN** — the Duress PIN hash file and its enabled flag were untouched by `disablePinEncryption()`, so a configured Duress PIN remained on disk and "active" even though startup PIN's lock screen — its only possible entry point — no longer existed. Disabling the startup PIN now also removes `/duress_pin.hash`.

### Security Fixes
- **PIN length disclosure mitigation** — PIN entry mask now displays fixed-width 
  (10 slots) regardless of configured PIN length, preventing visual shoulder-surfing 
  attacks. Previously, the number of dots/stars on screen revealed the exact PIN 
  length. Auto-submit timing may still reveal approximate length to a live observer 
  (documented known limitation, accepted UX trade-off for two-button interface).
- **Startup PIN can no longer be disabled while Hidden Space exists** — disabling
  the startup PIN silently attempted to wipe Hidden Space using the wrong key (see
  Fixed above) and would have left an inconsistent, partially-decrypted device
  state. Disabling PIN is now hard-blocked, both server-side (`400 pin_required`-style
  early rejection before the on-device prompt) and on-device, while Hidden Space is
  provisioned; the web cabinet greys out and disables the toggle with an inline
  explanation. Removing Hidden Space is a separate, explicit action that now requires
  entering Space B's own PIN on-device before any deletion occurs.
- **Hidden Space creation on an unencrypted device key** — `createHiddenSpace()`
  wrote its dual-slot structure at a fixed file offset without checking that the
  existing device key file was already in PIN-encrypted format. On a device with
  the startup PIN disabled (33-byte unencrypted key file), this silently corrupted
  the key file instead of failing. Added an `isDeviceKeyEncrypted()` guard at every
  entry point — all three web handlers, the on-device provisioning flow, and inside
  `createHiddenSpace()` itself as defense-in-depth — plus a greyed-out, disabled
  Hidden Space section in the web cabinet when the startup PIN is off.
- **Duress PIN configurable without startup PIN enabled** — `POST /api/duress_pin_update`
  had no dependency on startup PIN state, allowing a Duress PIN to be set up even
  though it can only ever be triggered from the startup PIN lock screen. Added an
  `isDeviceKeyEncrypted()` guard to all three web handlers (`400` early rejection),
  plus a greyed-out, disabled Duress PIN section in the web cabinet, matching the
  Hidden Space guard pattern above.

### Documentation
- Updated [API Endpoints](docs/development/ENDPOINTS.md) — added `send_login`, `login`, `nav_mode`, `login_delay_ms` parameters to `/api/passwords/*` endpoints; added `wildcard` and `wildcard_len` parameters with immutability notes for update operations
- Updated [User Guide (EN)](docs/user/GUIDE.html) — added "Send Login Before Password" section with navigation mode explanations; Wildcard Passwords section already present
- Updated [User Guide (RU)](docs/user/GUIDE.ru.html) — added "Отправка логина перед паролем" section; Wildcard-пароли section already present
- Updated [Decrypt Export Guide](docs/user/decrypt-export-guide.md) — Wildcard passwords section with copy/export restrictions already present
- Updated [Security Overview](docs/development/security/SECURITY_OVERVIEW.md) — 
  added PIN length timing disclosure to Known Limitations section; added wildcard password RAM zeroing limitation (`setCharAt()` vs `secure_memzero()` distinction)
- Updated [Security Model](docs/development/security/security_model.md) — 
  added PIN length via timing to Threat Model (Out of Scope); added comprehensive Wildcard Passwords section covering singleton constraint, session-based generation, RAM zeroing points, and platform limitations
- Updated [System Design](docs/development/system_design.md) — clarified 
  `/.sys_ui_prefs` file description regarding fixed-width mask implementation; added ENT, L/T, L/E, and RND badges to Password Security Badges table; updated `secureShutdown()` section with wildcard session wipe details
- Updated [Modes Guide](docs/user/MODES.md) — added note on wildcard password behavior consistency across network modes with export restriction clarification
- Updated [README.md](README.md) — added Wildcard passwords to Password Manager feature list
- Updated [ENDPOINTS.md](docs/development/ENDPOINTS.md) — replaced the legacy 6-endpoint import/export API reference with Secure Import/Export Mode's routes
- Updated [SECURITY_OVERVIEW.md](docs/development/security/SECURITY_OVERVIEW.md) — added "Import/export security model" section explaining the closed-AP, no-Layer-4 trade-off
- Updated [README.md](README.md) — clarified bulk import feature description to reference Secure Import/Export Mode
- Updated [Security Overview](docs/development/security/SECURITY_OVERVIEW.md) — corrected Hidden Space section: PIN disable is now blocked (not auto-wiped) while Hidden Space is provisioned
- Updated [Security Model](docs/development/security/security_model.md) — replaced references to `wipeHiddenSpace()` with `removeHiddenSpaceWithPin()`, corrected the calling-context description
- Updated [API Endpoints](docs/development/ENDPOINTS.md) — corrected `/api/hidden_space` disable action: removed the inaccurate "must be called from Space B context" claim, documented the new `prompt_on_device` deferred-confirmation response
- Updated [User Guide (EN)](docs/user/GUIDE.html) and [User Guide (RU)](docs/user/GUIDE.ru.html) — corrected Hidden Space section: PIN disable is blocked, not automatic, while Hidden Space is active
- Updated [Modes Guide](docs/user/MODES.md) — corrected Hidden Space constraint description to match blocked (not automatic-wipe) behavior

---

## [2.4.0] — June 2026
### Added
- **Hidden Space** — second independent encrypted vault selectable at boot via
  alternate PIN. Each space has isolated TOTP keys, passwords, web cabinet
  credentials, WiFi config, BLE PIN, Device BLE PIN, Duress PIN, display theme,
  startup mode, and HID mode. Spaces share only hardware-level settings: boot mode,
  RTC config, display settings, web server timeout, session duration, AP password,
  BLE device name, and mDNS hostname. Space B web cabinet hides all shared settings.
- **WiFi sharing** — optional: share Space A WiFi credentials with Space B
  via re-encryption with a chip-derived key (configurable in web cabinet,
  Space A context only).
- **Auto-Send (Enter)** — new per-password flag in Password Manager; when enabled, device automatically presses Enter via BLE/USB HID after typing the password. Configurable per entry in web cabinet and offline decrypt tool. ENT badge shown on device display.

### Fixed
- `quickUpdateCategory()` — quick category change via badge pill now preserves `auto_send` flag (previously reset to false on category switch)/
- Name of key file obfuscated
- PROGMEM copy bug(first web server open wasnt loading, loaded after page refresh)

## [2.3.0] — May 2026
### Security Enhancements
- **Duress PIN with memory zeroing** — multi-layer duress PIN triggers advanced memory 
  zeroing; real vault is wiped and replaced with decoy accounts on duress entry
- **Credential validation on registration and password change** — login name cannot 
  appear inside the password; enforced at registration and password change
- **Improved mimicry headers** — obfuscation layer now generates more convincing 
  decoy Authorization/Session headers; fixed header obfuscation logic bug

### Web Interface
- **Password categories (folders)** — passwords can be organized into named categories 
  in the web cabinet with folder-style navigation
- **Security badges** — visual indicators on password entries: strength meter, 
  duplicate detection, PIN-only flag, name-in-password warning
- **Debug UI** — internal diagnostics panel (debug builds only)
- **Display orientation setting** — screen rotation configurable from web cabinet
- **Display Auto-Dim** — screen dim configurable from web cabinet
- **BLE Pin Setting** — Deleted BLE pin setting, because it generates randoml. The setting was useless

### Bug Fixes & Stability
- Fixed BLE broadcasting page showing `0` instead of zero-padded PIN (e.g. `000006`)
- Fixed web server not stopping in AP mode on exit
- Fixed session lifecycle bug in AP mode (sessions persisting across AP restarts)
- Fixed URL obfuscation mapping bug causing endpoint resolution failures

### Documentation
- Added [RTC Mastering Guide](docs/user/rtc_mastering.md) — DS3231 wiring diagrams, 
  pin assignments for ESP32 and S3, enabling RTC in firmware, troubleshooting
- Added [Porting Guide](docs/development/PORTING.md) — step-by-step instructions for 
  adapting SecureGen to new ESP32/S3 boards: board header creation, platformio.ini 
  configuration, capability flags, deep sleep wake pins, display geometry, 
  security checklist, common mistakes

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
