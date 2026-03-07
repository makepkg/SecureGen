# ESP32 T-Display TOTP — Documentation

## Structure

### security/
- `SECURITY_OVERVIEW.md` — Public-facing security summary. Cryptographic standards, known limitations, threat model overview.
- `security-model.md` — Developer/auditor reference. Full 8-layer architecture, key hierarchy, threat model tables, new endpoint checklist.
- `pbkdf2-security.md` — PBKDF2 parameters, iteration counts, hardware constraints, compatibility notes for decrypt_export.html.

### development/
- `api-endpoints.md` — Complete API reference. All endpoints with auth requirements, request/response formats, security notation.
- `system-design.md` — Internal architecture. Boot sequence, PIN design, shutdown/deep sleep, data persistence table, error handling.

### user/
- `GUIDE.html` — Device operation manual.
- `MODES.md` — Network and display modes, feature matrix, mode selection at startup.
- `decrypt-export-guide.md` — Offline export decryption tool guide.

---

Last updated: March 2026