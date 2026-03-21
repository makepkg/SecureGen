# ESP32 T-Display Multifunctional Security Device

<div align="center">

**Open-source hardware security device featuring TOTP Authenticator and Password Manager with BLE Keyboard**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-Powered-blue.svg)](https://www.espressif.com/)

[![YouTube](https://img.shields.io/badge/YouTube-Demo-red?logo=youtube)](https://www.youtube.com/watch?v=YTVQBwgok_E)
[![Hackster](https://img.shields.io/badge/Hackster.io-Featured-00979D?logo=hackster)](https://www.hackster.io/makepkg/securegen-open-source-totp-authenticator-password-manager-c350d6)
[![Dev.to](https://img.shields.io/badge/Dev.to-Blog-0A0A0A?logo=dev.to)](https://dev.to/makepkg)
[![Product Hunt](https://img.shields.io/badge/Product_Hunt-Launched-DA552F?logo=producthunt&logoColor=white)](https://www.producthunt.com/products/securegen-2)
[![Flash Online](https://img.shields.io/badge/Flash-Online-brightgreen?logo=espressif)](https://makepkg.github.io/SecureGen/flash)
[![Languages](https://img.shields.io/badge/Languages-EN%20%7C%20RU%20%7C%20DE%20%7C%20ZH%20%7C%20ES-informational)](#)

[Video Demo](#-video-demo) • [Features](#-key-features) • [Installation](#-quick-start) • [Documentation](#-documentation) • [Security](#-security) • [Support](#-support)

</div>

---

## 📸 Device Gallery

### Physical Device
<table>
  <tr>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/totp-dark-mode.jpg" alt="TOTP Dark Theme" width="100%"/>
      <br/><b>TOTP Mode (Dark Theme)</b>
      <br/>Real-time authentication codes
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/totp-light-mode.jpg" alt="Light Theme" width="100%"/>
      <br/><b>TOTP Mode (Light Theme)</b>
      <br/>Customizable display themes
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/hotp-light-mode.jpg" alt="HOTP Light Theme" width="100%"/>
      <br/><b>HOTP Mode (Light Theme)</b>
      <br/>Counter-based authentication
    </td>
  </tr>
  <tr>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/password-manager-mode.jpg" alt="Password Manager" width="100%"/>
      <br/><b>Password Manager Mode</b>
      <br/>Secure offline password vault
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/ble-security-pin.jpg" alt="BLE Security" width="100%"/>
      <br/><b>BLE Security Mode</b>
      <br/>Encrypted wireless transmission
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/qr-code-export.jpg" alt="QR Export" width="100%"/>
      <br/><b>QR Code Export</b>
      <br/>Export keys directly from display
    </td>
  </tr>
  <tr>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/boot-mode-prompt.jpg" alt="Boot Mode Selection" width="100%"/>
      <br/><b>Boot Mode Selection</b>
      <br/>WiFi / AP / Offline at startup
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/battery-status.jpg" alt="Battery Status" width="100%"/>
      <br/><b>Battery & Status</b>
      <br/>Real-time monitoring
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/device-ambient.jpg" alt="Device Ambient" width="100%"/>
      <br/><b>Hardware Overview</b>
      <br/>Compact open-source design
    </td>
  </tr>
  <tr>
    <td align="center" width="33%">
      <img src="assets/screenshots/photos/factory-reset.jpg" alt="Factory Reset" width="100%"/>
      <br/><b>Factory Reset</b>
      <br/>Secure data wiping
    </td>
    <td align="center" width="33%"></td>
    <td align="center" width="33%"></td>
  </tr>
</table>

### Web Management Interface
<table>
  <tr>
    <td align="center" width="33%">
      <img src="assets/screenshots/screenshots/web-dashboard.png" alt="Web Dashboard" width="100%"/>
      <br/><b>Dashboard</b>
      <br/>Secure web access
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/screenshots/web-totp-management.png" alt="TOTP Management" width="100%"/>
      <br/><b>TOTP Management</b>
      <br/>QR code scanning & bulk import
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/screenshots/web-password-vault.png" alt="Password Vault" width="100%"/>
      <br/><b>Password Vault</b>
      <br/>Encrypted storage & search
    </td>
  </tr>
  <tr>
    <td align="center" width="33%">
      <img src="assets/screenshots/screenshots/web-security-settings.png" alt="Security Settings" width="100%"/>
      <br/><b>Security Settings</b>
      <br/>PIN & authentication config
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/screenshots/web-device-config.png" alt="Device Config" width="100%"/>
      <br/><b>Device Configuration</b>
      <br/>Network & display settings
    </td>
    <td align="center" width="33%">
      <img src="assets/screenshots/screenshots/pass-generation.png" alt="Password Generator" width="100%"/>
      <br/><b>Password Generator</b>
      <br/>Advanced generation & statistics
    </td>
  </tr>
</table>

---

## 🎥 Video Demo

<div align="center">

[![SecureGen Demo Video](https://img.youtube.com/vi/YTVQBwgok_E/maxresdefault.jpg)](https://www.youtube.com/watch?v=YTVQBwgok_E)

**Watch the full demonstration** — TOTP generation, password management, BLE keyboard, and web interface.

[▶️ Watch on YouTube](https://www.youtube.com/watch?v=YTVQBwgok_E)

</div>

---

## ✨ Key Features

### 🔐 TOTP / HOTP Authenticator
- Compatible with Google Authenticator, Microsoft Authenticator, Authy, and all RFC 6238 / RFC 4226 services
- SHA1 / SHA256 / SHA512, 6 and 8 digit codes, 30s and 60s periods
- HOTP counter-based codes with automatic counter increment
- Add keys via QR code scan (camera or file), manual entry, or bulk import
- Export any key as QR code — displayed on the device screen and in the web interface
- Encrypted storage with unique per-device key

### 🔑 Password Manager
- Offline encrypted vault — works without any network connection
- BLE HID keyboard: types passwords directly into any device, no clipboard
- PIN protection for BLE transmission
- Encrypted export/import for backup and migration

### 🌐 Web Management Interface
- Runs on the device itself — no cloud, no external servers
- Full TOTP and password management from any browser
- Password generator with complexity settings
- Three network modes: WiFi client, AP hotspot, or fully offline
- Multilingual interface — English, Russian, German, Chinese (Simplified), and Spanish

### 🎨 Display & Themes
- Light and dark themes, switchable from the web interface
- Custom splash screens on boot
- Battery indicator and WiFi status always visible

### ⚡ Hardware
- LILYGO® TTGO T-Display ESP32 — dual-core 240MHz, 1.14" TFT display
- Battery monitoring with real-time voltage and percentage
- Deep sleep and light sleep power saving
- **DS3231 RTC module support** — accurate offline timekeeping without WiFi; enables TOTP in AP and Offline modes

---

## 🛡️ Security

All sensitive data is encrypted with AES-256 using a unique per-device key derived from your PIN via PBKDF2-HMAC-SHA256. The web interface runs over an HTTPS-like encrypted channel (ECDH P-256 key exchange + AES-256-GCM) — works even in AP mode without certificates.

**8 layers of web protection:** key exchange → session encryption → URL obfuscation → header obfuscation → decoy traffic → method tunneling → timing protection → honeypot endpoints.

**Device security:** PIN with persistent lockout (5 attempts across reboots), secure memory wipe before deep sleep, encrypted BLE pairing.

### Known Limitations
- PBKDF2 iteration count (25,000) is below OWASP 2023 recommendations due to ESP32 hardware constraints
- No hardware secure enclave or secure boot by default
- Active MITM on initial ECDH exchange is not detectable without a server certificate

→ [Security Overview](docs/development/security/SECURITY_OVERVIEW.md) — full security summary  
→ [Security Model](docs/development/security/security_model.md) — technical reference for developers and auditors

---

## 🎮 Device Controls

| Button | Action | Function |
|--------|--------|----------|
| **Button 1** (Top) | Short press | Previous item |
| | Long press 2s | Switch TOTP ↔ Password Manager |
| **Button 2** (Bottom) | Short press | Next item |
| | Long press 5s | Power off (deep sleep) |
| **Both buttons** | 2s in Password Mode | Activate BLE keyboard |
| | 5s on PIN screen | Shutdown |
| | 5s on boot | Factory reset |

Wake from sleep: press Button 2.

---

## 🚀 Quick Start

### Requirements
- [PlatformIO](https://platformio.org/platformio-ide) (VS Code extension)
- LILYGO® TTGO T-Display ESP32
- USB-C cable

### ⚡ No tools? Flash from browser
[**→ Web Flasher**](https://makepkg.github.io/SecureGen/flash) — Chrome/Edge + USB, no install needed  
[**→ User Guide**](https://makepkg.github.io/SecureGen/guide)  
[**→ Decrypt Export Tool**](https://makepkg.github.io/SecureGen/tools)

### Install

```bash
git clone https://github.com/makepkg/SecureGen.git
cd SecureGen
# Open in VS Code with PlatformIO, then Upload
```

### First Boot

1. Device creates AP `ESP32-TOTP-Setup` → connect and open `192.168.4.1`
2. Enter WiFi credentials
3. Set administrator password and optional PIN
4. Device syncs time via NTP and is ready

→ [Complete User Manual](docs/user/GUIDE.html) for detailed setup and usage

---

## 📚 Documentation

| Document | Audience |
|----------|----------|
| [User Manual](docs/user/GUIDE.html) | All users — setup, operation, features |
| [Operating Modes](docs/user/MODES.md) | Network and display mode reference |
| [Decrypt Export Tool](docs/user/decrypt-export-guide.md) | Offline backup decryption |
| [Security Overview](docs/development/security/SECURITY_OVERVIEW.md) | Security summary |
| [Security Model](docs/development/security/security_model.md) | Full technical security reference |
| [API Endpoints](docs/development/ENDPOINTS.md) | Developer API reference |
| [System Design](docs/development/system_design.md) | Architecture and boot sequence |
| [Logging System](docs/development/LOGGING_SYSTEM.md) | Debug and log configuration |

---

## 🗺️ Roadmap

- Quick search by account name, favorites / pinned accounts, grouping by tags (work, personal, finance)
- Adapt firmware for other ESP32  (T-Display-S3)
- Informative setting flags in web cabinet
- Flash encryption and secure boot (optional hardening)
- Migration ECDH P-256 → X25519
- Support ATECC608 Secure module

---

## 🤝 Support & Community

- **Issues:** [GitHub Issues](https://github.com/makepkg/SecureGen/issues)
- **Discussions:** [GitHub Discussions](https://github.com/makepkg/SecureGen/discussions)
- **YouTube:** [Demo & Tutorials](https://www.youtube.com/watch?v=YTVQBwgok_E)
- **Dev.to:** [Technical Articles](https://dev.to/makepkg)
- **Twitter/X:** [@makepkg](https://x.com/makepkg_)
- **Hackster.io:** [Featured Project](https://www.hackster.io/makepkg/securegen-open-source-totp-authenticator-password-manager-c350d6)
- **Product Hunt:** [Launch Page](https://www.producthunt.com/products/securegen-2)

### Donations

[![Sponsor](https://img.shields.io/badge/Sponsor-❤-red.svg)](https://github.com/sponsors/makepkg)

**USDT BEP-20:** `0x4f85f29892b261fa8029f3cfd64211e166744733`  
**USDT TRC-20:** `TDnjDg9HxySo1J2FPSrvWQejyZ4gHKiXSJ`

**⭐ Star this repo if you find it useful!**

---

## 📄 License

MIT — see [LICENSE](LICENSE). Third-party: TFT_eSPI (FreeBSD), ESPAsyncWebServer (LGPL-3.0), AsyncTCP (LGPL-3.0), ArduinoJson (MIT), mbedTLS (Apache 2.0).

---

<div align="center">

**Made with ❤️ for the open-source community**

[⬆ Back to Top](#esp32-t-display-multifunctional-security-device)

</div>
