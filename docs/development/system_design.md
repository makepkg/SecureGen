# System Design

**Last updated:** March 2026
**Audience:** Developers

---

## Component Overview

```
Application Layer
├── TOTP/HOTP Generator
├── Password Manager
└── BLE HID Keyboard

Service Layer
├── Web Server
├── WiFi Manager
└── Display Manager

Security Layer
├── Crypto Manager
├── Secure Layer Manager
└── URL Obfuscation Manager

Platform
├── LittleFS
├── ESP32 Hardware
└── mbedTLS
```

**Manager responsibilities:**

| Manager | Responsibility |
|---------|---------------|
| `CryptoManager` | Device key, PBKDF2, AES, RNG, session storage |
| `SecureLayerManager` | ECDH key exchange, AES-256-GCM transport encryption |
| `KeyManager` | TOTP/HOTP key storage and code generation |
| `PasswordManager` | Password storage and retrieval |
| `PinManager` | PIN entry UI, BLE PIN configuration |
| `WifiManager` | WiFi client, AP mode, mDNS |
| `WebServerManager` | HTTP server, API endpoints, authentication |
| `ConfigManager` | Non-sensitive configuration persistence |
| `DisplayManager` | TFT display, UI rendering, themes |
| `BatteryManager` | ADC battery monitoring |
| `RTCManager` | DS3231 hardware RTC: config load/save, I2C init, time read/write, boot compensation |

---

## Boot Sequence

### Phase 1 — Hardware (0–100ms)

1. Serial init, LogManager init
2. GPIO init (BTN1 = GPIO35, BTN2 = GPIO0, INPUT_PULLUP)
3. Factory reset detection — both buttons held at boot
4. BatteryManager init

**Factory reset trigger:** Both buttons held for 5 seconds during boot. Deletes all user data files, NVS partition (BLE bonding keys), URL obfuscation mappings, PIN attempt counter. Device reboots into first-boot flow.

### Phase 2 — Filesystem (100–200ms)

5. LittleFS.begin() — formats on failure
6. Check for `/device.key` existence

If LittleFS mount fails: device halts with error screen.

### Phase 3 — Cryptographic init (200ms–boot complete)

7. `CryptoManager::begin()` — initializes mbedTLS CTR_DRBG with hardware entropy

**Device key path branches:**

```
/device.key missing  →  First boot: PIN creation + key generation
/device.key exists, encrypted  →  Normal boot: PIN entry
/device.key exists, unencrypted  →  Legacy: key loaded without PIN
```

**First boot (PIN creation):**
- User selects PIN length (4–10 digits)
- User enters and confirms PIN
- Device generates 32-byte random device key
- Key encrypted with PBKDF2(PIN) → AES-256-CBC → saved as 81-byte file
- Cannot proceed without completing PIN setup

**Normal boot (PIN entry):**
- Load persistent attempt counter from `/.pin_attempts`
- If counter ≥ 5: show "DEVICE LOCKED", enter deep sleep (factory reset required)
- PIN entry prompt shown on device screen
- Each failed attempt: increment and save counter to flash
- After 5 total failed attempts (across reboots): permanent lockout
- Successful unlock: counter file deleted

**PIN verification mechanism:** PIN is never stored. Verification works by attempting PBKDF2(PIN, salt) → AES-256-CBC decrypt → SHA256 checksum comparison. If checksum matches, PIN is correct and device key is loaded into RAM.

### Phase 4 — Security layer init

8. `SecureLayerManager::begin()` — ECDH context prepared
9. `WebAdminManager::begin()` — load registration and session state

### Phase 5 — Application init

10. Theme, BLE name, startup mode loaded from config
11. `KeyManager::begin()` — load encrypted TOTP/HOTP keys
12. `PasswordManager::begin()` — load encrypted passwords
13. `PinManager::begin()` — load PIN UI preferences
14. Splash screen (if enabled)

### Phase 5.5 — DS3231 RTC init (optional)

Between PIN unlock and mode selection, if DS3231 is enabled in config:

```
Wire.begin(sda_pin, scl_pin)
rtc.begin() → chip found?
  Yes → rtc.now().unixtime() > 1609459200?
    Yes → settimeofday() → system clock set from RTC
    No → battery dead or never calibrated → timeSynced = false
  No → LOG_ERROR, timeSynced = false
```

Config stored in `/rtc_config.json`: `{"enabled": bool, "sda_pin": int, "scl_pin": int}`  
Default pins: SDA=21, SCL=22. Custom pins applied via `reinit()` on-the-fly without reboot.

**Per-mode behaviour after RTC init:**

| Mode | RTC enabled + valid | RTC disabled or invalid |
|------|-------------------|------------------------|
| WiFi | Sets initial time, then NTP overwrites and saves back to RTC | NTP is mandatory source |
| AP | TOTP works immediately; user can re-sync via web cabinet | System clock zeroed → NOT SYNCED |
| Offline | TOTP works; re-sync on next AP/WiFi boot | NOT SYNCED |

**Pseudo-sleep re-sync:** On every wake from pseudo-sleep, if DS3231 is available, `syncFromRTC()` is called to correct ESP32 internal RTC drift accumulated during sleep. Note: pseudo-sleep reduces CPU to 40 MHz and suspends the TFT controller — it does not use `esp_light_sleep_start()` due to hardware incompatibility with battery power (voltage drop on CPU wake causes POWER_ON reset).

**NTP → RTC write-back (WiFi mode):** After successful NTP sync, time is written to DS3231 on a second boundary (busy-wait for `tv_sec` rollover) to minimize sub-second accumulation error.

### Phase 6 — Mode selection

15. `configManager.getBootMode()` → load saved default (`"wifi"` / `"ap"` / `"offline"`)
16. User prompt: two non-default modes shown as buttons (2-second timeout → saved default)
17. Button press resets timeout; BUTTON_2 confirms immediately

Default is configurable via web cabinet (Settings → Boot Mode). Factory default: `"wifi"`. Stored as `boot_mode` field in `/config.json`.

### Phase 7 — Network init (WiFi and AP modes only)

**WiFi mode:**
- Load encrypted WiFi credentials
- Connect, obtain IP via DHCP, start mDNS
- NTP sync (time.google.com → pool.ntp.org → time.cloudflare.com)
- Disconnect WiFi after sync (power saving)
- Prompt to start web server (5s timeout)

**AP mode:**
- Create access point `ESP32-TOTP-Setup` at 192.168.4.1
- Start DNS, mDNS
- Auto-start web server

**Offline mode:** Skip all network init.

### Phase 8 — Main loop

17. Watchdog timer init (10s timeout)
18. Load display timeout config
19. Enter main event loop

**Initialization dependency order:**
```
LittleFS → CryptoManager → Device Key → All encrypted data
                                      → SecureLayerManager
                                      → WebAdminManager
                                      → KeyManager, PasswordManager, PinManager
                                                                       → WebServer
```

---

## PIN Architecture

### Key hierarchy

```
PIN (never stored)
 └─► PBKDF2-HMAC-SHA256 (PBKDF2_ITERATIONS_PIN iterations)
          └─► PIN-derived key (32 bytes, never stored)
                   └─► AES-256-CBC decrypt
                            └─► Device key (32 bytes, RAM only)
                                     └─► AES-256-CBC encrypt/decrypt
                                              └─► All data files
```

### Device key file format

| Size | Version | Format |
|------|---------|--------|
| 81 bytes | v3 (current) | `[0x03][salt:16][IV:16][encrypted:48]` |
| 65 bytes | v2 (legacy) | `[0x02][salt:16][encrypted:48]` — AES-ECB |
| 49 bytes | v1 (legacy) | `[0x01][salt:16][encrypted:32]` — no checksum |
| ≤33 bytes | v0/ancient | Unencrypted |

Encrypted payload (48 bytes) contains: `[SHA256(device_key)[0:4]][device_key:32][padding:12]`

### PIN disable flow

PIN is never transmitted over the network. Disabling PIN via web cabinet works as follows:

1. Web interface sends disable request
2. Server sets in-memory flag `shouldPromptPinDisable = true`
3. Server responds: "Enter PIN on device to confirm"
4. Main loop detects flag, prompts PIN entry on physical device
5. User enters correct PIN on device
6. Device decrypts device key, saves it unencrypted (`[0x00][key:32]`)
7. Device reboots

### PIN enable flow

Enabling PIN requires factory reset because the existing device key must be replaced with one generated under proper entropy + PIN encryption:

1. Web interface requests PIN enable
2. Server checks: device key currently unencrypted
3. Requires factory reset confirmation
4. All data wiped, device reboots into first-boot PIN setup

---

## Shutdown and Deep Sleep

The device has no hard power-off. "Shutdown" means entering deep sleep via `esp_deep_sleep_start()`. Wake is via GPIO0 (▼ Bottom button) — configured via `esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0)` before every `esp_deep_sleep_start()` call. RST button also wakes the device.

### Trigger locations

| Trigger | Location | Condition |
|---------|---------|-----------|
| Hold BTN2 5 seconds | `src/main.cpp` | In TOTP or Password display mode |
| Hold both buttons 5 seconds | `src/pin_manager.cpp` | During PIN entry |
| PIN lockout | `src/main.cpp` | 5 failed attempts reached |
| Auto Lock timeout | `src/main.cpp` | Inside pseudo-sleep polling loop when `auto_lock_timeout > 0` |
| Auto Lock (screen=Never) | `src/main.cpp` | In main loop when `screen_timeout == 0` and `auto_lock_timeout > 0` |

### Secure shutdown

Before every `esp_deep_sleep_start()` call, `secureShutdown()` is called:

```cpp
void secureShutdown() {
    cryptoManager.wipeDeviceKey();      // memset _deviceKey[32]
    keyManager.wipeSecrets();           // zero + clear TOTP secrets
    passwordManager.wipePasswords();    // zero + clear passwords
    secureLayerManager.wipeAllSessions(); // zero session keys, free ECDH context
}
```

This wipes all sensitive data from RAM before the device enters deep sleep.

---

## Web Server Lifecycle

### Startup

| Mode | Web server |
|------|-----------|
| WiFi | User prompted at boot (5s timeout) |
| AP | Auto-starts always |
| Offline | Never starts |

Auto-start can be configured: `web_server_auto_start` in `config.json`.

### Timeout

Default: 10 minutes. Timer resets on any authenticated API call. On expiry: web server stops, WiFi disconnects. Restart requires device interaction.

### Sessions

Sessions are stored encrypted in `/session.json.enc`. They survive reboots. Duration is configurable (until reboot / 1h / 6h / 24h / 3 days). Cleared on logout or factory reset.

---

## Data Persistence

| File | Encryption | Contents |
|------|-----------|---------|
| `/device.key` | PIN-derived key | Encrypted device key |
| `/keys.json.enc` | Device key | TOTP/HOTP secrets |
| `/passwords.json.enc` | Device key | Passwords |
| `/wifi_config.json.enc` | Device key | WiFi credentials |
| `/session.json.enc` | Device key | Web session data |
| `/ble_pin.json.enc` | Device key | BLE PIN |
| `/pin_config.json` | Device key | BLE PIN enabled flag, config version |
| `/config.json` | None (AP password field encrypted) | Theme, timeouts, startup mode, boot mode |
| `/ble_config.json` | None | BLE device name |
| `/mdns_config.json` | None | mDNS hostname |
| `/.sys_ui_prefs` | None | PIN length (UI preference only) |
| `/.pin_attempts` | None | Failed PIN attempt counter (integer) |
| `/boot_counter.txt` | None | URL obfuscation epoch counter |
| `/rtc_config.json` | None | DS3231 RTC enabled flag, SDA/SCL pins |

URL obfuscation mappings are fully pre-generated at startup via `registerCriticalEndpoint()` rather than on-demand. This avoids repeated flash writes during normal operation — all 38 mappings are written once per epoch (every 30 reboots).

**Note:** `config.json` is plaintext but contains no secrets — the AP password field within it is individually encrypted. PIN length in `/.sys_ui_prefs` is not sensitive; it reduces brute-force search space marginally but PBKDF2 cost makes this irrelevant in practice.

---

## Error Handling

### Fatal (device halts or sleeps)

- LittleFS mount failure → error screen, halt
- PIN attempt limit reached → deep sleep, factory reset required
- Crypto init failure → halt

### Recoverable (logged, operation continues)

- WiFi connection failure → retry or AP mode prompt
- NTP sync failure on first server → try next server
- Web server start failure → continue without web
- Session load failure → require re-login

### User-recoverable

- Wrong PIN → retry (up to 5 total across reboots)
- WiFi credentials invalid → AP mode for reconfiguration
- Corrupted data file → factory reset