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
**Not available:** TOTP (no time sync), web cabinet

### AP Mode

Device creates its own WiFi access point (`ESP32-TOTP-Setup`, IP: 192.168.4.1). Web cabinet starts automatically.

**Available:** Passwords, HOTP codes, BLE password transfer, web cabinet  
**Not available:** TOTP (no NTP in AP mode)

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
| TOTP codes | ❌ | ❌ | ✅ |
| BLE password transfer | ✅ | ✅ | ✅ (web server off) |
| Web cabinet | ❌ | ✅ | ✅ (on demand) |
| NTP time sync | ❌ | ❌ | ✅ |

---

## Mode Selection at Startup

On boot, the device shows a mode selection prompt with a 3-second timeout. The default on timeout is WiFi Client. The selected mode is saved and restored on next boot.