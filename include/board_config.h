#pragma once

// ============================================================
// Board configuration router
//
// For T-Display S3:  board = lilygo-t-display-s3 in platformio.ini
//                    PlatformIO manifest auto-defines ARDUINO_LILYGO_T_DISPLAY_S3
//
// For T-Display ESP32: board = lilygo-t-display in platformio.ini
//                      Use -DTARGET_TDISPLAY_ESP32 in build_flags as guard
// ============================================================

#if defined(ARDUINO_LILYGO_T_DISPLAY_S3)
#include "boards/board_s3.h"
#elif defined(ARDUINO_LILYGO_T_DISPLAY) || defined(TARGET_TDISPLAY_ESP32)
#include "boards/board_esp32.h"
#else
#error "Unknown board. Set board = lilygo-t-display or lilygo-t-display-s3 in platformio.ini"
#endif
