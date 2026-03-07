# URL Obfuscation Architecture

## Overview

The URL Obfuscation system protects API endpoints from network traffic analysis by mapping real endpoint paths to randomly generated obfuscated paths. This makes it difficult for passive observers to understand the application's API structure or identify sensitive operations. Mappings rotate every 30 device reboots to limit the usefulness of captured traffic patterns.

## Endpoints

### Public Bootstrap: /api/client/config

- **Authentication:** None required
- **Purpose:** Provides minimal pre-auth URL mappings needed before login
- **Response format:** `{"k":"<keyexchange_path>","t":"<tunnel_path>","l":"<login_path>"}`
- **Keys:** 
  - `k` = keyexchange endpoint
  - `t` = tunnel endpoint
  - `l` = login endpoint
- **Called by:** `page_index.h`, `page_login.h`, `page_register.h` on every page load
- **Why public:** Client needs keyexchange URL before authentication exists. The keyexchange must happen before login, and login must happen before authentication, creating a bootstrap dependency chain.

### Debug Only: /api/obfuscation/mappings

- **Guard:** `#ifdef DEBUG_BUILD` only
- **Authentication:** None (debug builds only)
- **Used by:** `page_test_encryption.h`
- **Not available in production builds**
- **Purpose:** Exposes all 38 endpoint mappings for debugging and testing

## Pre-auth vs Post-auth Mappings

- **Pre-auth (3 mappings via /api/client/config):** keyexchange, tunnel, login
  - These are the minimum required to establish secure communication and authenticate
  - Available to unauthenticated clients for bootstrap
  
- **Post-auth (all 38 mappings):** 
  - In DEBUG builds: loaded via `/api/obfuscation/mappings`
  - In production: client uses only the 3 pre-auth mappings and routes all API calls through the tunnel endpoint
  - Tunnel endpoint handles method obfuscation and routing internally

## Rotation

- **Rotation frequency:** Every 30 reboots (epoch change)
- **Pre-generation:** All 38 endpoints pre-generated at startup via `registerCriticalEndpoint()`
- **Storage files:** 
  - `/url_mappings_<epoch>.json` - Mapping data for specific epoch
  - `/boot_counter.txt` - Persistent reboot counter
- **Cleared on:** Factory reset
- **No on-the-fly generation:** All mappings created during initialization to avoid excessive flash writes

## Files

- **src/url_obfuscation_manager.cpp** - Core manager, generation, rotation logic
- **src/url_obfuscation_integration.cpp** - Helper functions for dual endpoint registration
- **src/web_server.cpp** - `/api/client/config` handler implementation
- **include/web_pages/page_index.h** - Main page consumer (lines ~4546)
- **include/web_pages/page_login.h** - Login page consumer (lines ~207)
- **include/web_pages/page_register.h** - Register page consumer (lines ~214)

## Security Notes

- `/api/client/config` intentionally public (bootstrap requirement)
- Full mappings never exposed publicly in production builds
- Neutral endpoint name (`client/config`) hides purpose from network observers
- Rotation every 30 reboots limits usefulness of captured mappings
- Minimal response size (3 mappings) reduces information leakage
- All post-auth API calls routed through single tunnel endpoint for additional obfuscation
