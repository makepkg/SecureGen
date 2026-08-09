# Device Modes

The device has two independent mode dimensions: the **display mode** (what is shown on screen) and the **network mode** (connectivity level). They are selected independently at startup.

---

## Display Modes

### TOTP / HOTP Authenticator

Shows time-based (TOTP) and counter-based (HOTP) codes for stored keys. Navigate between keys with the buttons. TOTP codes require accurate time — only available with NTP sync (WiFi Client mode). HOTP codes work in all network modes.

### Password Manager

Shows stored passwords. Navigate and select entries with the buttons. Passwords can be transmitted as hardware keyboard input — no app required on the receiving device. Hold both buttons for 2 seconds to start transmission.

On **T-Display ESP32** (classic): BLE HID only.  
On **T-Display-S3**: BLE HID or USB HID — a prompt appears on screen to select the output mode before transmission begins.

#### HID Output Prompt (T-Display-S3 only)

On T-Display-S3, holding both buttons shows a prompt to choose between BLE HID and USB HID before transmission:

- The device pre-selects the **default mode** (configurable in web cabinet → Settings → HID Mode)
- Wait for the **auto-selection timer** to confirm the default, or press a button to switch to the other mode
- Transmission begins immediately after selection

USB HID requires no pairing or drivers — the device is recognized as a standard keyboard on any OS.

---

## Network Modes

### Offline

WiFi disabled. Maximum battery life.

**Available:** Passwords, HOTP codes, BLE password transfer  
**Not available:** Web cabinet  
**TOTP:** Requires DS3231 RTC module (hardware clock). If enabled and calibrated — TOTP codes work fully. If disabled or not connected — shows NOT SYNCED.

### AP Mode

Device creates its own WiFi access point (`ESP32-TOTP-Setup`, IP: 192.168.4.1). Web cabinet starts automatically.

**Available:** Passwords, HOTP codes, BLE password transfer, web cabinet  
**TOTP:** Requires DS3231 RTC module. If enabled — sync time via web cabinet (Display Settings → DS3231 RTC → Sync & Save). Browser clock is used as time source. After sync, TOTP codes appear immediately without reboot.

**Session behavior:** Web cabinet sessions work normally in AP mode. Without DS3231 RTC, session persistence is limited to the current boot — timed Auto-Logout modes (1 h, 6 h, 24 h, 3 days) automatically fall back to "until reboot" behavior. With RTC enabled and time synced, sessions persist across reboots according to the configured Auto-Logout Timer.

### WiFi Client

Connects to an existing WiFi network. Full functionality.

**Available:** Passwords, TOTP codes, HOTP codes, web cabinet (on demand)  
**Note:** BLE password transfer is disabled while the web server is running. Stop the web server first, or use BLE in Offline/AP mode.

---

## Feature Matrix

| Feature | Offline | AP Mode | WiFi Client |
|---------|---------|---------|-------------|
| Passwords (on-device) | ✅ | ✅ | ✅ |
| HOTP codes | ✅ | ✅ | ✅ |
| TOTP codes | ⚠️ DS3231 | ⚠️ DS3231 | ✅ |
| BLE password transfer | ✅ | ✅ | ✅ (web server off) |
| USB HID transfer (S3 only) | ✅ | ✅ | ✅ (web server off) |
| DS3231 RTC time sync | ✅ | ✅ (via web cabinet) | ✅ (auto via NTP) |
| Web cabinet | ❌ | ✅ | ✅ (on demand) |
| NTP time sync | ❌ | ❌ | ✅ |

**Note:** Wildcard passwords (randomly generated, marked with 🎲 **RND** badge) work identically in all modes for on-device display and HID transmission. However, they cannot be copied to clipboard or exported via web cabinet — they are designed to stay on-device only. See [Wildcard Passwords](../user/decrypt-export-guide.md#wildcard-passwords) for details.

---

## Mode Selection at Startup

On boot, after PIN unlock, the device shows a mode selection prompt on the display.

**Prompt behavior:**
- Two buttons show the two non-default network modes
- Timeout: **2 seconds** of inactivity → device boots into the default mode
- Any button press resets the timeout (gives another 2 seconds)
- BUTTON_1: toggle between the two shown options
- BUTTON_2: confirm selection immediately

**Default mode:**
The default mode on timeout is configurable. Factory default is WiFi Client.

To change it: open the web cabinet → **Settings** → **Boot Mode** → select preferred mode → Save Boot Mode.

The selected default is stored persistently in device config. After factory reset, default reverts to WiFi Client.

**Example — default set to AP Mode:**
- Display shows: `Auto: AP Mode (default)` with buttons `[WiFi]` `[Offline]`
- No interaction → AP Mode starts
- Press BUTTON_1 to highlight WiFi → BUTTON_2 to confirm → WiFi Client starts instead

---

## Space Selection at Boot 

If Hidden Space is configured, boot adds a prior step before network mode 
selection: the PIN entry determines which space loads. 

| PIN entered | Result | 
|-------------|--------| 
| Space A PIN | Primary vault — all Space A data, full web cabinet | 
| Space B PIN | Hidden vault — isolated Space B data, separate web cabinet | 
| Wrong PIN (5× total across reboots) | Factory reset | 

Space selection is invisible from the outside — the device shows the same 
PIN entry screen regardless of which space is configured. An observer cannot 
determine how many spaces exist or which PIN was entered. 

**Space B characteristics at first boot:** 
- No TOTP keys, no passwords, no WiFi credentials (unless WiFi sharing enabled) 
- Web cabinet requires fresh registration 
- BLE PIN disabled by default 
- Network mode selection and display mode work identically to Space A 

**Constraints:** 
- Hidden Space requires startup PIN to be active. Disabling startup PIN 
  is blocked entirely while Space B exists — the web cabinet toggle is 
  greyed out, and the on-device flow refuses with an on-screen message. 
  Space B must be removed first (a separate action requiring Space B's 
  own PIN on-device) before the startup PIN can be disabled. 
- Factory reset can only be initiated from Space A. 
- RTC module config and time sync are shared — both spaces benefit equally 
  from a connected DS3231. 

### Feature Matrix — Per-Space Isolation 

| Data / Setting | Space A | Space B | Shared | 
|----------------|---------|---------|--------| 
| TOTP / HOTP keys | ✅ own | ✅ own | — | 
| Passwords | ✅ own | ✅ own | — | 
| Web cabinet account | ✅ own | ✅ own | — | 
| WiFi credentials | ✅ own | ✅ own (or shared) | optional | 
| BLE PIN | ✅ own | ✅ own | — | 
| Device BLE PIN | ✅ own | ✅ own | — | 
| Duress PIN | ✅ own | ✅ own | — | 
| Display theme | ✅ own | ✅ own | — | 
| Startup mode | ✅ own | ✅ own | — | 
| HID mode (BLE/USB) | ✅ own | ✅ own | — | 
| BLE device name | — | — | ✅ | 
| mDNS hostname | — | — | ✅ | 
| Boot mode | — | — | ✅ | 
| RTC configuration | — | — | ✅ | 
| Display settings | — | — | ✅ | 
| Web server timeout | — | — | ✅ | 
| Session duration | — | — | ✅ | 
| AP password | — | — | ✅ |

**Note:** Space B web cabinet hides all shared (global) device settings. Only Space A can modify boot mode, RTC, display settings, web server timeout, session duration, AP password, BLE device name, and mDNS hostname.

---

### Display Mode (Startup Mode)

The device can be configured to open either TOTP Authenticator or Password Manager by default after PIN unlock.

To change it: open the web cabinet → **Settings** → **Startup Mode** → select preferred mode → Save Startup Mode.

| Setting | Behavior |
|---------|----------|
| TOTP Authenticator (default) | Device opens on TOTP/HOTP code screen |
| Password Manager | Device opens on password list screen |

The selected startup mode is stored persistently in device config. After factory reset, default reverts to TOTP Authenticator.

**Note:** Startup Mode is independent of Network Mode — both settings are applied on each boot.