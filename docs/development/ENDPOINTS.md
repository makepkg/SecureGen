# API Endpoints

**Last updated:** March 2026
**Adding new endpoints:** see checklist in [`docs/security/security-model.md`](../security/security-model.md#new-endpoint-checklist)

All endpoints operate over HTTP with application-level encryption (AES-256-GCM). Logical paths are mapped to obfuscated hex paths at runtime — use `/api/client/config` to resolve pre-auth paths, and the tunnel endpoint for all others after key exchange.

---

## Security notation

| Symbol | Meaning |
|--------|---------|
| 🔐 | Session cookie required |
| 🛡️ | CSRF token required |
| 🔒 | Transported through encrypted tunnel |

All authenticated endpoints are also URL-obfuscated and method-tunneled unless noted otherwise.

---

## Bootstrap (public)

### GET /api/client/config
No auth required. Returns the three obfuscated paths needed to initiate a session (3 of 38 total mappings). All other mappings are available only after authentication — in production builds, all post-auth API calls are routed through the single tunnel endpoint so the client never needs the full mapping table.

```json
{ "k": "<keyexchange_path>", "t": "<tunnel_path>", "l": "<login_path>" }
```

### GET /api/obfuscation/mappings
Available in DEBUG builds only (`#ifdef DEBUG_BUILD`). No authentication required. Returns all 38 endpoint mappings for development and testing. Not present in production builds.

### POST /api/secure/keyexchange 🔐 🔗
ECDH P-256 key exchange. Client sends ephemeral public key, server responds with its public key. Both sides derive the AES-256 session key independently via HKDF.

Request: `{ "client_id": "...", "client_public_key": "04..." }`  
Response: `{ "server_public_key": "04..." }`

---

## Authentication

### GET /register · GET /login
Public HTML pages. `/register` redirects to `/login` if a user already exists. `/login` redirects to `/` if already authenticated.

### POST /register
Public. Creates the admin account. Only works if no user is registered.  
Request: `{ "username": "...", "password": "..." }`  
Password is hashed with PBKDF2-HMAC-SHA256 (`PBKDF2_ITERATIONS_LOGIN` iterations, ~2.7s).

### POST /login 🔗
Public path (received via bootstrap). Verifies password, creates encrypted session, sets `HttpOnly; SameSite=Strict` cookie.  
Request: `{ "username": "...", "password": "..." }`

### POST /logout 🔐 🔒
Destroys session cookie and deletes the encrypted session file.

---

## TOTP / HOTP Keys

### GET /api/keys 🔐 🔒
Returns list of all keys. Secrets are not included in the response.

```json
{
  "keys": [
    { "name": "GitHub", "type": "T", "algorithm": "SHA1", "digits": 6, "period": 30 }
  ]
}
```

`type`: `"T"` = TOTP, `"H"` = HOTP.

### POST /api/add 🔐 🛡️ 🔒
Adds a new TOTP or HOTP key.

```json
{ "name": "...", "secret": "BASE32...", "type": "T", "algorithm": "SHA1", "digits": 6, "period": 30 }
```

### POST /api/remove 🔐 🛡️ 🔒
Deletes key by index.  
Request: `{ "index": 0 }`

### POST /api/show_qr 🔐 🛡️ 🔒
Triggers QR code display on device screen for 30 seconds. Returns the `otpauth://` URI.  
Request: `{ "index": 0 }`  
Response: `{ "success": true, "uri": "otpauth://totp/..." }`

### POST /api/hotp/generate 🔐 🛡️ 🔒
Increments HOTP counter and returns the new code.  
Request: `{ "index": 0 }`  
Response: `{ "success": true, "code": "123456", "counter": 5 }`

### POST /api/keys/reorder 🔐 🛡️ 🔒
Reorders keys. `order` is an array of current indices in desired order.  
Request: `{ "order": [2, 0, 1] }`

---

## Passwords

### GET /api/passwords 🔐 🔒
Returns password list metadata. Passwords are not included — use `/api/passwords/get` to retrieve a specific entry.

```json
{
  "passwords": [
    {
      "name": "Gmail",
      "category": "web",
      "strength": 2,
      "pw_hash": "a1b2c3d4",
      "auto_send": false
    }
  ]
}
```

| Field | Type | Description |
|-------|------|-------------|
| name | string | Password entry name |
| category | string | Category: "web", "app", "local", "key", or "" |
| strength | int | Password strength: 0=unknown, 1=weak, 2=medium, 3=strong |
| pw_hash | string | First 8 bytes of SHA-256 (hex) for duplicate detection |
| auto_send | bool | Whether Enter is sent after password output |
| send_login | bool | Whether login is sent before password via HID |
| login | string | Login value (used only when send_login=true) |
| nav_mode | string | Navigation method after login: "enter" or "tab" (default "enter") |
| login_delay_ms | int | Delay in ms after nav_mode key before typing password (default 300, only relevant when nav_mode="enter") |
| wildcard | bool | Whether this is a wildcard (randomly-generated on-device) password. When true, the password field contains placeholder and the real value is never transmitted or stored |
| wildcard_len | int | Length of generated wildcard password (4-64, default 16). Only relevant when wildcard=true |

Passwords are never included in the list response — use `/api/passwords/get`.

### POST /api/passwords/add 🔐 🛡️ 🔒
Request: `{ "name": "...", "password": "...", "category": "web", "auto_send": false }`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| name | string | required | Password entry name |
| password | string | required | Password value |
| category | string | "" | Category: "web", "app", "local", "key", or "" |
| auto_send | bool | false | Automatically press Enter after HID output |
| send_login | bool | false | Send login before password via HID |
| login | string | "" | Login value (used only when send_login=true) |
| nav_mode | string | "enter" | Navigation method after login: "enter" or "tab" |
| login_delay_ms | int | 300 | Delay in ms after nav_mode key before typing password (only relevant when nav_mode="enter") |
| wildcard | bool | false | Mark entry as wildcard (randomly generated on-device). Only one wildcard entry allowed. When true, name and password parameters are system-managed |
| wildcard_len | int | 16 | Length of generated wildcard password (4-64). Only relevant when wildcard=true |

Note: `category`, `auto_send`, `send_login`, `login`, `nav_mode`, `login_delay_ms`, `wildcard`, and `wildcard_len` are optional.

### POST /api/passwords/get 🔐 🛡️ 🔒
Returns the plaintext password for one entry.  
Request: `{ "index": 0 }`  
Response: `{ "success": true, "password": "...", "name": "...", "category": "web", "auto_send": false }`

| Response Field | Type | Description |
|----------------|------|-------------|
| success | bool | Operation status |
| password | string | Plaintext password value |
| name | string | Password entry name |
| category | string | Category: "web", "app", "local", "key", or "" |
| auto_send | bool | Whether Enter is sent after password output |
| send_login | bool | Whether login is sent before password via HID |
| login | string | Login value (used only when send_login=true) |
| nav_mode | string | Navigation method after login: "enter" or "tab" |
| login_delay_ms | int | Delay in ms after nav_mode key before typing password (only relevant when nav_mode="enter") |
| wildcard | bool | Whether this is a wildcard entry (randomly generated on-device) |
| wildcard_len | int | Length of generated wildcard password (4-64) |

### POST /api/passwords/update 🔐 🛡️ 🔒
Request: `{ "index": 0, "name": "...", "password": "...", "category": "web", "auto_send": false }`

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| index | int | required | Password entry index |
| name | string | required | Password entry name |
| password | string | required | Password value |
| category | string | "" | Category: "web", "app", "local", "key", or "" |
| auto_send | bool | false | Automatically press Enter after HID output |
| send_login | bool | false | Send login before password via HID |
| login | string | "" | Login value (used only when send_login=true) |
| nav_mode | string | "enter" | Navigation method after login: "enter" or "tab" |
| login_delay_ms | int | 300 | Delay in ms after nav_mode key before typing password (only relevant when nav_mode="enter") |
| wildcard | bool | N/A | Cannot be changed after creation — wildcard status is immutable |
| wildcard_len | int | 16 | Length of generated wildcard password (4-64). For wildcard entries, this is the ONLY editable field |

**Note:** `send_login`, `login`, `nav_mode`, and `login_delay_ms` are optional per-entry fields. When `send_login` is enabled and `login` is non-empty, the device types the login value first, then presses Tab or Enter (per `nav_mode`), waits `login_delay_ms` (only relevant for `nav_mode="enter"`), then types the password. A `LOGIN` badge (with an Enter ⏎ or Tab ⇥ icon) is shown on the device screen and in the web cabinet for entries with `send_login` enabled.

**Wildcard entry restrictions:** If the entry at `index` is a wildcard entry, only `wildcard_len` can be modified. All other parameters (`name`, `password`, `category`, `auto_send`, `send_login`, `login`, `nav_mode`, `login_delay_ms`) are ignored and existing values are preserved. This is enforced in `PasswordManager::updatePassword()`.

### POST /api/passwords/delete 🔐 🛡️ 🔒
Request: `{ "index": 0 }`

### POST /api/passwords/reorder 🔐 🛡️ 🔒
Reorder the password list.

**Request:** 
```json
{ "order": [2, 0, 1] }
```
Array of original indices in desired new order.

---

## Import / Export

Import and export of TOTP keys and passwords is handled entirely through
**Secure Import/Export Mode** — an isolated, closed-AP session separate
from the ordinary web cabinet. There are no import/export endpoints in
the ordinary authenticated API surface; the feature is deliberately kept
out of the 8-layer web cabinet stack (see
[`SECURITY_OVERVIEW.md`](security/SECURITY_OVERVIEW.md) for the
rationale).

### Entering the mode

### POST /api/enter_import_export_mode 🔐 🛡️ 🔒
Authenticated, ordinary-cabinet endpoint. Requires the admin web password
(re-verified via `WebAdminManager::verifyCredentials()`, independent of
the current session) in addition to normal auth/CSRF. On success, writes
a flag file tagged with the currently-active Hidden Space (`A` or `B`)
and reboots the device.

Request: `{ "password": "..." }`  
Response (success): `{ "status": "rebooting" }`  
Response (failure): `{ "error": "..." }`

The device reboots, requires a normal PIN unlock, and — only if the
unlocked space matches the space that requested the mode — boots into
a dedicated, closed WPA2 access point (`ESP32-IMPEXP-XXXX`) instead of
the ordinary web cabinet.

### Restricted-mode routes (served only inside the closed AP session)

These routes exist **only** while the device is inside Secure
Import/Export Mode — they are not reachable from the ordinary network
connection, and the ordinary cabinet's 100+ routes are not reachable
from inside this AP. No CSRF, no session cookie, no ECDH/AES-256-GCM
transport layer — security here comes from the closed AP (freshly
generated WPA2 password, RAM-only, never persisted) plus the prior PIN
unlock, not from Layer 4. See `SECURITY_OVERVIEW.md` for this trade-off.

- `GET /` — serves the restricted-mode page (own HTML/CSS/JS, does not
  reuse the ordinary cabinet's `page_index.h`)
- `POST /api/restricted/export_keys` — exports TOTP keys, password-
  encrypted (`PBKDF2_ITERATIONS_EXPORT`, AES-256-CBC, same format as
  `decrypt_export.html` expects)
- `POST /api/restricted/export_passwords` — exports passwords, same
  encryption scheme
- `POST /api/restricted/import_keys` — imports a previously exported
  keys file; validates the decrypted payload contains a `secret` field
  on every entry before writing, rejecting mismatched file types (e.g.
  a passwords export submitted here) with an explicit error instead of
  silently corrupting the key store
- `POST /api/restricted/import_passwords` — imports a previously
  exported passwords file; same schema validation, requiring a
  `password` field on every entry
- `POST /api/restricted/close_session` — ends the session and reboots
  back to normal operation

All operations resolve file paths via `CryptoManager::getSpacePath(...)`,
so they automatically operate on whichever Hidden Space is active —
never hardcoded to Space A.

**Session bounds:** 10-minute rolling inactivity timeout, 30-minute hard
cap regardless of activity. Either limit expiring, or a manual "Close
Session," reboots the device back to the ordinary cabinet.

**Minimum export/import password length:** 8 characters. This password
is separate from both the device PIN and the web admin password.

---

## Configuration

### GET /api/config 🔐 🔒
Response: `{ "timeout": 10, "auto_start": false }`  
`timeout` is web server auto-shutdown in minutes.

### GET|POST /api/theme 🔐 (🛡️ on POST) 🔒
Get or set display theme.  
Values: `"dark"`, `"light"`.

### GET|POST /api/display_settings 🔐 (🛡️ on POST) 🔒
Get or set screen timeout and auto lock timeout.

GET response:
```json
{ "display_timeout": 30, "auto_lock_timeout": 300, "dim_timeout": 15 }
```

`display_timeout` — seconds until screen turns off. Valid values: `0`, `15`, `30`, `60`, `300`, `1800`.

`auto_lock_timeout` — seconds until device enters deep sleep and wipes RAM. Valid values: `0`, `300`, `900`, `1800`, `3600`, `14400`.

`dim_timeout` — seconds until display brightness drops to 20% (auto-dim). Valid values: `0` (disabled), `5`, `10`, `15`, `30`, `60`, `300`. Constraint: `dim_timeout < display_timeout < auto_lock_timeout` (when each is non-zero).

POST request: `{ "display_timeout": 30, "auto_lock_timeout": 300, "dim_timeout": 15 }`  
POST response: `{ "success": true, "message": "Display settings saved successfully!", "timeout": 30, "auto_lock_timeout": 300, "dim_timeout": 15 }`

### GET|POST /api/clock_settings 🔐 (🛡️ on POST) 🔒
Get or set POSIX timezone string.  
Example: `{ "timezone": "EST5EDT,M3.2.0,M11.1.0" }`

### GET|POST /api/display/rotation 🔐 (🛡️ on POST) 🔒
Get or set the screen rotation.

**GET response:**
```json
{ "rotation": 1 }
```

**POST request:**
```json
{ "rotation": 3 }
```

Values: `1` = Normal landscape (default, USB on right), `3` = Flipped 180°.  
Full range `0–3` supported via API (0=portrait, 2=portrait inverted) but only 1 and 3 are exposed in the web UI.

Change applies immediately — display redraws and button mappings swap automatically.  
Available on both T-Display ESP32 and T-Display-S3.

### GET|POST /api/ble_settings 🔐 (🛡️ on POST) 🔒
Get or set BLE device name.

### GET|POST /api/mdns_settings 🔐 (🛡️ on POST) 🔒
Get or set mDNS hostname (used as `<hostname>.local`).

### GET|POST /api/session_duration 🔐 (🛡️ on POST) 🔒
Get or set session lifetime in hours.  
Options: until reboot, 1, 6, 24, 72.

### GET|POST /api/boot-mode 🔐 (🛡️ on POST) 🔒
Get or set the default network mode used on boot timeout.

GET response: `{ "boot_mode": "wifi" }`  
POST request: `{ "boot_mode": "wifi" }` — accepted values: `"wifi"`, `"ap"`, `"offline"`.  
POST response: `{ "success": true, "boot_mode": "wifi" }`

The selected mode becomes the timeout default during the boot prompt (2-second window). The other two modes remain selectable via physical buttons. Takes effect on next reboot. Factory default: `"wifi"`.

### GET|POST /api/hid-mode 🔐 🛡️ 🔒
**S3 only.** Get or set the default HID output mode used when sending passwords via hardware keyboard emulation.

GET response: `{ "hid_mode": "ble" }`  
POST request: `{ "hid_mode": "usb" }` — accepted values: `"ble"`, `"usb"`.  
POST response: `{ "success": true, "hid_mode": "usb" }`  
Error: `{ "error": "Invalid hid_mode" }`

The setting determines which mode is pre-selected when the device shows the HID output prompt (triggered by holding both buttons in PASSWORD mode). The user can either wait for the auto-selection based on this default, or press a button to switch to the other mode before transmission begins.

> Note: CSRF token is required on both GET and POST — unlike most read-only GET endpoints.  
> Stored in `config.json` as `default_hid_mode`. Factory default: `"ble"`.

---

## Security Settings

### GET /api/pincode_settings 🔐 🔒
Response: `{ "device_pin_enabled": true, "ble_pin_enabled": false, "pin_length": 6 }`

### POST /api/pincode_settings 🔐 🛡️ 🔒
Enable requires factory reset confirmation. Disable requires physical PIN entry on device.

### POST /api/ble_pin_update 🔐 🛡️ 🔒
Request: `{ "ble_pin_enabled": true, "ble_pin": "123456" }`

### POST /api/duress_pin_update 🔐 🛡️ 🔒
Set or disable the Duress PIN.

**Enable / set:**
```json
{ "duress_pin_enabled": "true", "duress_pin": "123456" }
```

**Disable:**
```json
{ "duress_pin_enabled": "false" }
```

The Duress PIN must be the same length as the current Startup PIN (4–10 digits) and must consist of digits only. When entered at startup, the device shows "PIN OK" and then permanently erases all data (keys, passwords, device key, WiFi, BLE NVS, sessions, config) before restarting.
The entire LittleFS partition (~3.9 MB) is also wiped at the hardware level (`esp_partition_erase_range`) — file recovery via flash reader is not possible.

**Prerequisite:** Startup PIN must be enabled. The Duress PIN can only ever be triggered from the startup PIN lock screen, so it has no meaning without one — `POST /api/duress_pin_update` returns `400` if the startup PIN is disabled, and disabling the startup PIN automatically deletes any configured Duress PIN.

Status fields returned by `GET /api/pincode_settings`:
- `duressPinEnabled` — `true` / `false`
- `duressPinConfigured` — whether `/duress_pin.hash` exists and is valid

**Response 200:** `{ "success": true, "message": "Duress PIN saved successfully!" }`  
**Response 400:** `{ "success": false, "message": "Duress PIN must be N digits" }`

### POST /api/change_password 🔐 🛡️ 🔒
Request: `{ "current_password": "...", "new_password": "..." }`

### POST /api/wifi_credentials 🔐 🛡️ 🔒

Updates the WiFi client credentials used when connecting to an external network.  
Does not affect the current connection — changes apply after reboot.

Request:
```json
{
  "ssid": "MyNetwork",
  "password": "secret123",
  "confirm_password": "secret123"
}
```

Validation: `ssid` required; `password` must match `confirm_password`; if password  
is non-empty, minimum 8 characters (empty password = open network).

Response 200: `{ "success": true, "message": "WiFi credentials saved. Reboot to apply." }`  
Response 400: `{ "success": false, "message": "..." }` — validation error  
Response 500: `{ "success": false, "message": "Failed to save WiFi credentials" }`

### POST /api/change_ap_password 🔐 🛡️ 🔒
Request: `{ "new_password": "..." }`

---

## Hidden Space 

### GET /api/hidden_space 🔐 🔒 

Returns current hidden space status. Response differs by active space. 

**Space A response:** 
```json 
{ 
  "hidden_space_enabled": true, 
  "current_space": "A", 
  "can_enable": true, 
  "can_disable": true, 
  "share_wifi": false 
} 
``` 

**Space B response:** 
```json 
{ 
  "hidden_space_enabled": true, 
  "current_space": "B", 
  "can_enable": false, 
  "can_disable": true, 
  "share_wifi": false 
} 
``` 

`hidden_space_enabled` reflects whether Space B is provisioned (sentinel 
file detected). `share_wifi` is only meaningful in Space A context. 

### POST /api/hidden_space 🔐 🛡 🔒 

Manages hidden space lifecycle. All actions require Space A context except 
`disable` (which requires Space B context). 

**Action: enable** 
Writes `/.setup_hidden_space` trigger file and restarts the device. 
On next boot, PIN entry flow creates Space B slot. 

```json 
{ "action": "enable" } 
``` 

**Action: disable** 
Removes Space B: overwrites slot B with random bytes, deletes all 
HMAC-derived data files, removes sentinel and shared cache. Called from 
Space A context; requires Space B's own PIN, entered on-device, to 
correctly locate and delete its files. The HTTP response does not confirm 
deletion — it only signals that the device is now waiting for PIN entry. 

```json 
{ "action": "disable" } 
```

Response (immediate, before device confirmation): 
```json 
{ "status": "prompt_on_device", "success": true, "message": "Please enter Space B PIN on device to confirm. Check device screen." } 
```

The device then prompts for Space B's PIN on-screen. On success it deletes 
the data and reboots; on a wrong PIN, no data is touched and Hidden Space 
remains active. 

**Action: set_share_wifi** 
Copies Space A WiFi credentials re-encrypted with chip-derived key to 
`/.conn_cache`. Space B WifiManager uses this file as fallback if its own 
credentials are absent. Calling with `enabled: false` removes the cache file. 
Space A context only. 

```json 
{ "action": "set_share_wifi", "enabled": true } 
``` 

Response (all actions on success): 
```json 
{ "status": "ok" } 
``` 

> **Registration:** this endpoint follows the 6-location checklist 
> (direct handler, two tunnel dispatchers, obfuscation manager, 
> `shouldTunnelEndpoint`, `shouldSecureEndpoint`).

---

## Utility

### GET /api/csrf_token 🔐 🔒
Returns CSRF token for the current session.  
Response: `{ "csrf_token": "..." }`

### POST /api/activity 🔐 🔒
Resets the web server auto-shutdown timer. Called periodically by the frontend.

### POST /api/clear_ble_clients 🔐 🛡️ 🔒
Clears all BLE bonded devices.

### GET /api/battery 🔐 🔒
Returns current battery status. Polled by the web UI every 30 seconds.

Response 200:
```json
{ "level": 87, "charging": false }
```

`level` — integer 0–100. Derived from voltage range 3200–3800 mV mapped linearly.  
`charging` — bool. `true` if measured voltage exceeds 4.15 V (threshold-based, no dedicated pin).

Response 503: `{ "error": "Battery manager not available" }`

> Note: CSRF token is not required — this is a read-only GET endpoint. Authentication
> is verified at the tunnel dispatcher outer level, not inside the endpoint handler.

---

## Error responses

```json
{ "success": false, "message": "..." }
```

| Code | When |
|------|------|
| 400 | Invalid input or malformed JSON |
| 401 | Not authenticated |
| 403 | CSRF token missing or invalid |
| 404 | Endpoint not found |
| 500 | Server-side failure |