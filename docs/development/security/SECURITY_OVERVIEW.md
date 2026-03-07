# Security Overview

**Last updated:** March 2026
**Audience:** Public

---

## What is protected and how

All sensitive data on the device is encrypted at rest using AES-256-CBC with a unique device key. Nothing is stored in plaintext.

**Encrypted data:**
- TOTP and HOTP secrets
- Stored passwords
- WiFi credentials
- Web session data
- BLE pairing information

The device key itself is protected by your PIN through a two-level scheme: the PIN is processed through PBKDF2-HMAC-SHA256 to derive an encryption key, which is used to encrypt the device key file. The PIN and the derived key are never stored — only the encrypted device key file exists on disk. On boot, the correct PIN is required to reconstruct the derived key, decrypt the device key, and load it into RAM. Without the correct PIN, none of the data is accessible — including to someone who extracts the filesystem from the chip.

---

## Web interface security

The web interface runs over HTTP, not HTTPS. Standard TLS is not used because the ESP32's available RAM makes a full TLS handshake impractical alongside the rest of the firmware. Instead, the following layers are applied at the application level:

**ECDH + AES-256-GCM** — client and server perform an elliptic curve key exchange (P-256) on each session. The shared secret is processed through HKDF to derive a 256-bit session key. All subsequent request and response bodies are encrypted with AES-256-GCM. GCM mode provides both confidentiality and integrity: any modification to an encrypted message causes authentication tag validation to fail and the message is rejected. This was tested — substituting the tag causes decryption to fail explicitly on both sides.

**URL obfuscation** — API endpoint paths are mapped to randomly generated 12-character hex paths, rotated every 30 reboots. A passive observer cannot enumerate the API from traffic alone.

**Method tunneling** — all HTTP methods are sent as POST to a single obfuscated endpoint. From the network, all API traffic looks uniform.

**Header obfuscation** — two real headers are renamed to innocuous-looking names, and five fake decoy headers are added to make traffic resemble normal browser requests. Some metadata is optionally embedded in the User-Agent string. This is a traffic analysis resistance measure, not an encryption feature.

**Traffic obfuscation** — the device periodically sends decoy requests at randomized intervals, making it harder to determine when real user activity is occurring.

**Authentication** — passwords are hashed with PBKDF2-HMAC-SHA256 with a unique salt per user. Sessions use 32-byte random IDs and CSRF tokens. All state-changing operations require CSRF validation.

---

## Bluetooth security

BLE uses LE Secure Connections with PIN-based pairing (MITM protection). The pairing PIN is displayed on the device screen — the connecting device must enter it. After bonding, subsequent connections do not require re-entry.

An optional additional PIN can be configured that the device itself will ask for before transmitting any password over BLE. This prevents unauthorized transmission if someone has physical access to the device while it is unlocked.

BLE password transfer is not available when the web server is running.

---

## Physical security

The startup PIN encrypts the master device key. Disabling the PIN leaves the key file unencrypted on the filesystem — if the device is stolen in this state, stored data can be extracted.

Factory reset (RST + immediate double button hold) wipes all data including keys, passwords, WiFi credentials, sessions, and PIN. There is no recovery after a factory reset without a prior export backup.

---

## Known limitations

**PBKDF2 iterations** are lower than OWASP 2023 server-grade recommendations (600,000+). The ESP32 hardware triggers watchdog timeouts above ~30,000 iterations. Current values are the practical ceiling for this platform. A strong PIN and web password compensate partially — longer credentials increase brute-force time exponentially regardless of iteration count.

**HTTP not HTTPS** — the application-layer encryption described above is not equivalent to TLS in all threat scenarios. It provides strong protection against passive network observers and active tampering (GCM tag validation), but has not undergone the same scrutiny as production TLS implementations. Specifically: without a server certificate, an active attacker who intercepts and replaces the initial ECDH key exchange on the local network cannot be detected. Passive interception and ciphertext tampering in transit are mitigated.

**No secure boot** — the firmware is not signed. A malicious firmware image could be flashed without detection. Verify firmware source before flashing.

**No tamper detection** — the hardware has no mechanism to detect physical tampering.

---

## Responsible disclosure

If you find a security issue, please report it privately via GitHub before public disclosure. Allow reasonable time for a fix. For critical vulnerabilities, use private contact rather than a public issue.

---

## Cryptographic standards used

- AES-256-CBC — data at rest (FIPS 197)
- AES-256-GCM — application-layer encryption (NIST SP 800-38D)
- PBKDF2-HMAC-SHA256 — key derivation (RFC 2898)
- ECDH P-256 — session key exchange (FIPS 186-4)
- HKDF-HMAC-SHA256 — session key derivation from ECDH shared secret (RFC 5869)
- CTR_DRBG — random number generation (NIST SP 800-90A)
- RFC 6238 — TOTP (Time-Based One-Time Password)
- RFC 4226 — HOTP (HMAC-Based One-Time Password)
- RFC 2104 — HMAC


Compatible authenticator apps: Google Authenticator, Microsoft Authenticator, Authy, 1Password, and any RFC 6238/4226 compliant app.

---

## Recommendations

**Use AP mode for initial setup.** When registering your admin account for the first time, prefer AP mode over an external WiFi network. AP mode keeps traffic isolated to a direct connection between you and the device.

**Use a trusted network.** If you access the web interface over WiFi, make sure it is your own network. On a compromised network, an active attacker could intercept the initial key exchange — see Known Limitations above.

**Keep the PIN enabled.** Disabling the PIN removes hardware-level encryption of the device key. Without a PIN, the key file is stored unencrypted — anyone with physical access to the chip can extract all data. If you must disable the PIN, use Offline mode on boot to minimize exposure.

**Reset sessions on every reboot.** In Settings, configure session duration to "until reboot". This ensures no session survives a power cycle, reducing the window for session hijacking.

**Make regular encrypted backups.** Use the export function to save an encrypted backup. Store it in a safe place — it is the only way to recover your data after a factory reset or hardware failure.

**Use strong credentials.** The web admin password and PIN are your primary defense. Longer credentials increase brute-force time exponentially regardless of PBKDF2 iteration count.
