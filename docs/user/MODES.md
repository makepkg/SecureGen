# Device Modes

The device has two independent mode dimensions: the **display mode** (what is shown on screen) and the **network mode** (connectivity level). They are selected independently at startup.

---

## Display Modes

### TOTP / HOTP Authenticator

Shows time-based (TOTP) and counter-based (HOTP) codes for stored keys. Navigate between keys with the buttons. TOTP codes require accurate time — only available with NTP sync (WiFi Client mode). HOTP codes work in all network modes.

### Password Manager

Shows stored passwords. Navigate and select entries with the buttons. Passwords can be transmitted via BLE HID keyboard emulation (hold both buttons 2 seconds) — no app required on the receiving device.

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
| DS3231 RTC time sync | ✅ | ✅ (via web cabinet) | ✅ (auto via NTP) |
| Web cabinet | ❌ | ✅ | ✅ (on demand) |
| NTP time sync | ❌ | ❌ | ✅ |

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

### Display Mode (Startup Mode)

The device can be configured to open either TOTP Authenticator or Password Manager by default after PIN unlock.

To change it: open the web cabinet → **Settings** → **Startup Mode** → select preferred mode → Save Startup Mode.

| Setting | Behavior |
|---------|----------|
| TOTP Authenticator (default) | Device opens on TOTP/HOTP code screen |
| Password Manager | Device opens on password list screen |

The selected startup mode is stored persistently in device config. After factory reset, default reverts to TOTP Authenticator.

**Note:** Startup Mode is independent of Network Mode — both settings are applied on each boot.