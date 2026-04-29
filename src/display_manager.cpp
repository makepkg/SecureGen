#include "display_manager.h"
#include "config.h"
#include "log_manager.h"
#include <qrcode_espi.h>

// Helper for the animation loop
void schedule_next_update(DisplayManager* dm, AnimationManager* am);

void animation_callback(float val, bool finished, DisplayManager* dm, AnimationManager* am) {
    dm->updateHeader();
    if (finished) {
        schedule_next_update(dm, am);
    }
}

void schedule_next_update(DisplayManager* dm, AnimationManager* am) {
    am->startAnimation(20, 0.0f, 1.0f, [dm, am](float val, bool finished) {
        animation_callback(val, finished, dm, am);
    });
}


DisplayManager::DisplayManager() : tft(TFT_eSPI()), animationManager(), headerSprite(&tft), totpContainerSprite(&tft), totpSprite(&tft) {
    _currentThemeColors = &DARK_THEME_COLORS;
    _totpState = TotpState::IDLE;
    _lastDrawnTotpString = "";
    _lastScrambleFrameTime = 0;
    _totpContainerNeedsRedraw = true;
}

void DisplayManager::setTheme(Theme theme) {
    LOG_INFO("DisplayManager", "setTheme() called with theme: " + String((theme == Theme::LIGHT) ? "LIGHT" : "DARK"));
    switch (theme) {
        case Theme::DARK:
            _currentThemeColors = &DARK_THEME_COLORS;
            break;
        case Theme::LIGHT:
            _currentThemeColors = &LIGHT_THEME_COLORS;
            break;
    }
    LOG_DEBUG("DisplayManager", "Applying new theme colors to display...");
    tft.fillScreen(_currentThemeColors->background_dark);
    updateHeader(); 
    lastDisplayedCode = ""; 
    lastTimeRemaining = -1;
    _lastDrawnTotpString = ""; 
    _totpState = TotpState::IDLE;
    _totpContainerNeedsRedraw = true; // Force redraw of container with new theme
    LOG_INFO("DisplayManager", "Theme applied successfully");
}

void DisplayManager::update() {
#ifdef BOARD_HAS_USB_HID
    if (_usbHidMode) return;
#endif
    // Check for QR code request (thread-safe)
    if (_qrCodeRequested) {
        _qrCodeRequested = false;
        showQRCode(_qrCodeText, _qrCodeTimeoutSeconds);
    }
    
    // Check QR code timeout and update timer
    if (_qrCodeActive) {
        unsigned long currentTime = millis();
        if (currentTime >= _qrCodeTimeout) {
            hideQRCode();
        } else {
            // Update timer every second
            int secondsRemaining = (_qrCodeTimeout - currentTime) / 1000;
            if (secondsRemaining != _lastQRTimerSeconds) {
                _lastQRTimerSeconds = secondsRemaining;
                updateQRTimer(secondsRemaining);
            }
        }
    }
    
    if (!_qrCodeActive) {
        animationManager.update();
    }

    if (_totpState == TotpState::IDLE) {
        return;
    }

    unsigned long currentTime = millis();
    const unsigned long frameInterval = 30; 

    if (currentTime - _lastScrambleFrameTime < frameInterval) {
        return;
    }
    _lastScrambleFrameTime = currentTime;

    if (_totpContainerNeedsRedraw) {
        drawTotpContainer();
    }

    unsigned long elapsedTime = currentTime - _totpAnimStartTime;
    const unsigned long scrambleDuration = 300;
    const unsigned long revealDuration = 150;
    const unsigned long totalDuration = scrambleDuration + revealDuration;

    if (elapsedTime >= totalDuration) {
        _totpState = TotpState::IDLE;
        drawTotpText(_newCode);
        _currentCode = _newCode;
        return;
    }

    String textToDraw = "";
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    int codeLen = _newCode.length() > 0 ? _newCode.length() : 6;

    if (elapsedTime < scrambleDuration) {
        for (int i = 0; i < codeLen; i++) {
            textToDraw += charset[random(sizeof(charset) - 1)];
        }
    } else {
        int charsToReveal = (elapsedTime - scrambleDuration) / 25;
        if (charsToReveal > codeLen) charsToReveal = codeLen;
        textToDraw = _newCode.substring(0, charsToReveal);
        for (int i = charsToReveal; i < codeLen; i++) {
            textToDraw += charset[random(sizeof(charset) - 1)];
        }
    }
    
    drawTotpText(textToDraw);
}


// Ранняя инициализация для splash screen (минимальная подготовка)
void DisplayManager::initForSplash() {
#ifdef LCD_POWER_ON_PIN
    pinMode(LCD_POWER_ON_PIN, OUTPUT);
    digitalWrite(LCD_POWER_ON_PIN, HIGH);
    delay(10);
#endif
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK); // Очищаем экран чёрным для splash
    tft.setTextDatum(MC_DATUM);
    
    // ⚠️ ВАЖНО: Настройка PWM ПОСЛЕ tft.init() т.к. init() сбрасывает пин на digitalWrite!
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttach(TFT_BL, 5000, 8);
    ledcWrite(TFT_BL, 0);
#else
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, 0);
#endif
}

// Полная инициализация (для обычного UI)
void DisplayManager::init() {
#ifdef LCD_POWER_ON_PIN
    pinMode(LCD_POWER_ON_PIN, OUTPUT);
    digitalWrite(LCD_POWER_ON_PIN, HIGH);
    delay(10);
#endif
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(_currentThemeColors->background_dark); 
    tft.setTextDatum(MC_DATUM);
    
    // ⚠️ ВАЖНО: Настройка PWM ПОСЛЕ tft.init() т.к. init() сбрасывает пин!
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttach(TFT_BL, 5000, 8);
    ledcWrite(TFT_BL, 255);
#else
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, 255);
#endif

    headerSprite.createSprite(tft.width(), 35);
    headerSprite.setTextDatum(MC_DATUM);

    batterySprite.createSprite(30, 20);
    batterySprite.setColorDepth(16);

    // Создание спрайтов для TOTP
    int padding = 10;
    tft.setTextSize(4);
    int codeAreaWidth = tft.textWidth("888888") + padding * 2;
    int codeAreaHeight = 40 + 10;
    
    // Спрайт контейнера (с тенью)
    totpContainerSprite.createSprite(codeAreaWidth + 2, codeAreaHeight + 2);
    totpContainerSprite.setTextDatum(MC_DATUM);

    // Спрайт для текста (помещается внутри рамки)
    totpSprite.createSprite(codeAreaWidth - 2, codeAreaHeight - 2);
    totpSprite.setTextDatum(MC_DATUM);


    _totpState = TotpState::IDLE;
    _lastDrawnTotpString = "";
    _totpContainerNeedsRedraw = true;

    schedule_next_update(this, &animationManager);
}

void DisplayManager::drawLayout(const String& serviceName, int batteryPercentage, bool isCharging, bool isWebServerOn) {
    // Если до этого была страница "нет ключей", очищаем экран полностью
    if (_isNoItemsPageActive) {
        tft.fillScreen(_currentThemeColors->background_dark);
        _isNoItemsPageActive = false;
    } else if (_isKeySwitched) {
        // Иначе, если ключ был переключен, очищаем только область под заголовком
        tft.fillRect(0, headerSprite.height(), tft.width(), tft.height() - headerSprite.height(), _currentThemeColors->background_dark);
    }
    
    _currentServiceName = serviceName;
    _currentBatteryPercentage = batteryPercentage;
    _isCharging = isCharging;
    _isWebServerOn = isWebServerOn;
    _headerState = HeaderState::INTRO;
    _introAnimStartTime = millis();

    lastDisplayedCode = ""; 
    lastTimeRemaining = -999;
    _lastDrawnTotpString = "";
    _totpState = TotpState::IDLE;
    _totpContainerNeedsRedraw = true;
    // _isKeySwitched флаг устанавливается в main.cpp
}

void DisplayManager::updateBatteryStatus(int percentage, bool isCharging) {
    _currentBatteryPercentage = percentage;
    _isCharging = isCharging;

    if (_headerState != HeaderState::INTRO) {
        if (_isCharging) {
            if (_headerState != HeaderState::CHARGING) {
                _headerState = HeaderState::CHARGING;
                _chargingAnimStartTime = millis();
            }
        } else {
            _headerState = HeaderState::STATIC;
        }
    }
    
    // 🔋 Обновляем индикатор батареи на странице "No keys found" без полной перерисовки
    if (_isNoItemsPageActive) {
        int batteryX = tft.width() - 28;
        int batteryY = 5;
        int batteryWidth = 22;
        int batteryHeight = 12;
        int batteryCornerRadius = 3;
        int shadowOffset = 1;
        
        // Очищаем область батареи
        tft.fillRect(batteryX - 2, batteryY - 2, batteryWidth + 6, batteryHeight + 4, _currentThemeColors->background_dark);
        
        // Тень батареи
        tft.drawRoundRect(batteryX + shadowOffset, batteryY + shadowOffset, batteryWidth, batteryHeight, batteryCornerRadius, _currentThemeColors->shadow_color);
        tft.fillRect(batteryX + batteryWidth + shadowOffset, batteryY + 3 + shadowOffset, 2, 5, _currentThemeColors->shadow_color);
        
        // Обводка батареи
        tft.drawRoundRect(batteryX, batteryY, batteryWidth, batteryHeight, batteryCornerRadius, _currentThemeColors->text_secondary);
        tft.fillRect(batteryX + batteryWidth, batteryY + 3, 2, 5, _currentThemeColors->text_secondary);
        
        // Заполнение батареи
        int displayPct = percentage;
        if (isCharging) {
            unsigned long elapsed = millis() - _chargingAnimStartTime;
            int progress = (elapsed % 1500) * 100 / 1500;
            displayPct = (100 * progress) / 100;
        }
        
        uint16_t barColor;
        if (displayPct > 50) barColor = _currentThemeColors->accent_primary;
        else if (displayPct > 20) barColor = _currentThemeColors->accent_secondary;
        else barColor = _currentThemeColors->error_color;
        
        tft.fillRect(batteryX + 2, batteryY + 2, batteryWidth - 4, batteryHeight - 4,
                     _currentThemeColors->background_dark);
        
        int barWidth = map(displayPct, 0, 100, 0, batteryWidth - 4);
        if (barWidth > 0) {
            tft.fillRect(batteryX + 2, batteryY + 2, barWidth, batteryHeight - 4, barColor);
        }
        
        if (isCharging) {
            int bx = batteryX + batteryWidth / 2;
            int by = batteryY + 1;
            tft.drawPixel(bx + 1, by,     _currentThemeColors->text_primary);
            tft.drawPixel(bx,     by + 1, _currentThemeColors->text_primary);
            tft.drawPixel(bx + 1, by + 1, _currentThemeColors->text_primary);
            tft.drawPixel(bx,     by + 2, _currentThemeColors->text_primary);
            tft.drawPixel(bx + 1, by + 2, _currentThemeColors->text_primary);
            tft.drawPixel(bx - 1, by + 3, _currentThemeColors->text_primary);
            tft.drawPixel(bx,     by + 3, _currentThemeColors->text_primary);
        }
    }
}

void DisplayManager::updateHeader() {
    // Не отрисовываем заголовок если активен лоадер, чтобы предотвратить наслоение
    if (_loaderActive) {
        return;
    }
    
#ifdef BOARD_HAS_USB_HID
    if (_usbHidMode) {
        return;
    }
#endif
    
    // 🚫 Не отрисовываем заголовок на странице "No keys found" - шторка загораживает обводку!
    if (_isNoItemsPageActive) {
        if (_headerState == HeaderState::CHARGING) {
            unsigned long elapsed = millis() - _chargingAnimStartTime;
            int progress = (elapsed % 1500) * 100 / 1500;
            const int BW = 22, BH = 12, CR = 3, SO = 1;
            const int SX = 2, SY = 2; // sprite internal offset
            
            batterySprite.fillSprite(_currentThemeColors->background_dark);
            
            // shadow
            batterySprite.drawRoundRect(SX + SO, SY + SO, BW, BH, CR,
                                        _currentThemeColors->shadow_color);
            batterySprite.fillRect(SX + BW + SO, SY + 3 + SO, 2, 5,
                                   _currentThemeColors->shadow_color);
            
            // outline
            batterySprite.drawRoundRect(SX, SY, BW, BH, CR,
                                        _currentThemeColors->text_secondary);
            batterySprite.fillRect(SX + BW, SY + 3, 2, 5,
                                   _currentThemeColors->text_secondary);
            
            // fill
            batterySprite.fillRect(SX + 2, SY + 2, BW - 4, BH - 4,
                                   _currentThemeColors->background_dark);
            
            uint16_t barColor;
            if (progress > 50) barColor = _currentThemeColors->accent_primary;
            else if (progress > 20) barColor = _currentThemeColors->accent_secondary;
            else barColor = _currentThemeColors->error_color;
            
            int barWidth = map(progress, 0, 100, 0, BW - 4);
            if (barWidth > 0) {
                batterySprite.fillRect(SX + 2, SY + 2, barWidth, BH - 4, barColor);
            }
            
            // lightning bolt
            int bx = SX + BW / 2;
            int by = SY + 1;
            batterySprite.drawPixel(bx + 1, by,     _currentThemeColors->text_primary);
            batterySprite.drawPixel(bx,     by + 1, _currentThemeColors->text_primary);
            batterySprite.drawPixel(bx + 1, by + 1, _currentThemeColors->text_primary);
            batterySprite.drawPixel(bx,     by + 2, _currentThemeColors->text_primary);
            batterySprite.drawPixel(bx + 1, by + 2, _currentThemeColors->text_primary);
            batterySprite.drawPixel(bx - 1, by + 3, _currentThemeColors->text_primary);
            batterySprite.drawPixel(bx,     by + 3, _currentThemeColors->text_primary);
            
            batterySprite.pushSprite((int)tft.width() - 28 - SX, 5 - SY);
        }
        return;
    }
    
    // 🚫 Не отрисовываем заголовок когда показывается QR код
    if (_qrCodeActive) {
        return;
    }
    
    headerSprite.fillSprite(_currentThemeColors->background_dark);

    float titleY = 20;
    
    if (_headerState == HeaderState::INTRO) {
        unsigned long elapsedTime = millis() - _introAnimStartTime;
        if (elapsedTime >= 350) {
            titleY = 20;
            _headerState = _isCharging ? HeaderState::CHARGING : HeaderState::STATIC;
            if (_isCharging) _chargingAnimStartTime = millis();
        } else {
            float progress = (float)elapsedTime / 350.0f;
            titleY = -20.0f + (40.0f * progress);
        }
    }

    headerSprite.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_dark);
    headerSprite.setTextSize(2);
    headerSprite.drawString(_currentServiceName, headerSprite.width() / 2, (int)titleY);

    if (_headerState == HeaderState::CHARGING) {
        unsigned long chargeElapsedTime = millis() - _chargingAnimStartTime;
        int chargeProgress = (chargeElapsedTime % 1500) * 100 / 1500;
        drawBatteryOnSprite(_currentBatteryPercentage, true, chargeProgress);
    } else {
        drawBatteryOnSprite(_currentBatteryPercentage, false);
    }

    drawClockOnSprite();
    headerSprite.pushSprite(0, 0);
}

void DisplayManager::drawClockOnSprite() {
    struct tm timeinfo;
    bool synced = getLocalTime(&timeinfo, 0);

    // 7-segment digit: 5px wide, 9px tall, 1px lines
    // Segments: A=top, B=top-right, C=bot-right, D=bot, E=bot-left, F=top-left, G=mid
    // Digit origin at (dx, dy)
    auto drawDigit = [&](int dx, int dy, int d, uint16_t col) {
        uint16_t bg = _currentThemeColors->background_dark;
        // Clear digit area first
        headerSprite.fillRect(dx, dy, 5, 9, bg);
        if (d < 0 || d > 9) {
            // Draw '--' dash (segment G only)
            headerSprite.drawFastHLine(dx + 1, dy + 4, 3, col);
            return;
        }
        // Segment A (top)
        if (d==0||d==2||d==3||d==5||d==6||d==7||d==8||d==9)
            headerSprite.drawFastHLine(dx+1, dy,   3, col);
        // Segment B (top-right)
        if (d==0||d==1||d==2||d==3||d==4||d==7||d==8||d==9)
            headerSprite.drawFastVLine(dx+4, dy+1, 3, col);
        // Segment C (bot-right)
        if (d==0||d==1||d==3||d==4||d==5||d==6||d==7||d==8||d==9)
            headerSprite.drawFastVLine(dx+4, dy+5, 3, col);
        // Segment D (bot)
        if (d==0||d==2||d==3||d==5||d==6||d==8||d==9)
            headerSprite.drawFastHLine(dx+1, dy+8, 3, col);
        // Segment E (bot-left)
        if (d==0||d==2||d==6||d==8)
            headerSprite.drawFastVLine(dx,   dy+5, 3, col);
        // Segment F (top-left)
        if (d==0||d==4||d==5||d==6||d==8||d==9)
            headerSprite.drawFastVLine(dx,   dy+1, 3, col);
        // Segment G (mid)
        if (d==2||d==3||d==4||d==5||d==6||d==8||d==9)
            headerSprite.drawFastHLine(dx+1, dy+4, 3, col);
    };

    uint16_t col = _currentThemeColors->text_secondary;
    int y = 4; // top-aligned, matches battery icon at y=5

    if (!synced) {
        // Show "--:--" when not synced
        drawDigit(4,  y, -1, col);
        drawDigit(11, y, -1, col);
        drawDigit(21, y, -1, col);
        drawDigit(28, y, -1, col);
    } else {
        int h = timeinfo.tm_hour;
        int m = timeinfo.tm_min;
        drawDigit(4,  y, h / 10, col);
        drawDigit(11, y, h % 10, col);
        drawDigit(21, y, m / 10, col);
        drawDigit(28, y, m % 10, col);
    }

    // Colon: 2 pixels between HH and MM
    headerSprite.drawPixel(18, y + 2, col);
    headerSprite.drawPixel(18, y + 6, col);
}

void DisplayManager::updateClockStatus() {
    if (!_isNoItemsPageActive) return;  // На обычных страницах спрайт сам обновляет часы

    struct tm timeinfo;
    bool synced = getLocalTime(&timeinfo, 0);
    int h = synced ? timeinfo.tm_hour : -1;
    int m = synced ? timeinfo.tm_min  : -1;

    // Рисуем только если минута изменилась
    static int _lastClockH = -2;
    static int _lastClockM = -2;
    if (h == _lastClockH && m == _lastClockM) return;
    _lastClockH = h;
    _lastClockM = m;

    uint16_t col = _currentThemeColors->text_secondary;
    uint16_t bg  = _currentThemeColors->background_dark;
    int cy = 4;

    auto drawDigitTft = [&](int dx, int dy, int d) {
        tft.fillRect(dx, dy, 5, 9, bg);
        if (d < 0 || d > 9) { tft.drawFastHLine(dx+1, dy+4, 3, col); return; }
        if (d==0||d==2||d==3||d==5||d==6||d==7||d==8||d==9) tft.drawFastHLine(dx+1, dy,   3, col);
        if (d==0||d==1||d==2||d==3||d==4||d==7||d==8||d==9) tft.drawFastVLine(dx+4, dy+1, 3, col);
        if (d==0||d==1||d==3||d==4||d==5||d==6||d==7||d==8||d==9) tft.drawFastVLine(dx+4, dy+5, 3, col);
        if (d==0||d==2||d==3||d==5||d==6||d==8||d==9) tft.drawFastHLine(dx+1, dy+8, 3, col);
        if (d==0||d==2||d==6||d==8) tft.drawFastVLine(dx,   dy+5, 3, col);
        if (d==0||d==4||d==5||d==6||d==8||d==9) tft.drawFastVLine(dx,   dy+1, 3, col);
        if (d==2||d==3||d==4||d==5||d==6||d==8||d==9) tft.drawFastHLine(dx+1, dy+4, 3, col);
    };

    drawDigitTft(4,  cy, h < 0 ? -1 : h / 10);
    drawDigitTft(11, cy, h < 0 ? -1 : h % 10);
    drawDigitTft(21, cy, m < 0 ? -1 : m / 10);
    drawDigitTft(28, cy, m < 0 ? -1 : m % 10);
    tft.drawPixel(18, cy+2, col);
    tft.drawPixel(18, cy+6, col);
}

void DisplayManager::drawBatteryOnSprite(int percentage, bool isCharging, int chargingValue) {
    int x = headerSprite.width() - 28;
    int y = 5;
    int width = 22;
    int height = 11;
    int shadowOffset = 1;
    int cornerRadius = 3;

    headerSprite.drawRoundRect(x + shadowOffset, y + shadowOffset, width, height, cornerRadius, _currentThemeColors->shadow_color);
    headerSprite.fillRect(x + width + shadowOffset, y + 3 + shadowOffset, 2, 5, _currentThemeColors->shadow_color);

    headerSprite.drawRoundRect(x, y, width, height, cornerRadius, _currentThemeColors->text_secondary);
    headerSprite.fillRect(x + width, y + 3, 2, 5, _currentThemeColors->text_secondary);

    uint16_t barColor;
    int barWidth;

    // Determine bar color
    if (percentage > 50) barColor = _currentThemeColors->accent_primary;
    else if (percentage > 20) barColor = _currentThemeColors->accent_secondary;
    else barColor = _currentThemeColors->error_color;

    // Calculate bar width
    // When charging: animate from current percentage up to 100% using chargingValue (0-100 cycle)
    int displayPercentage;
    if (isCharging && chargingValue >= 0) {
        // Animate fill level from real percentage toward 100%
        int remaining = 100 - percentage;
        displayPercentage = percentage + (remaining * chargingValue / 100);
        barColor = _currentThemeColors->accent_primary;  // Always green when charging
    } else {
        displayPercentage = percentage;
    }
    
    barWidth = map(displayPercentage, 0, 100, 0, width - 4);
    if (barWidth < 0) barWidth = 0;

    // Clear fill area first (avoid ghost pixels)
    headerSprite.fillRect(x + 2, y + 2, width - 4, height - 4, _currentThemeColors->background_dark);

    // Draw fill bar
    if (barWidth > 0) {
        headerSprite.fillRect(x + 2, y + 2, barWidth, height - 4, barColor);
    }

    // Draw charging indicator: small lightning bolt pixel pattern
    if (isCharging) {
        int bx = x + width / 2;
        int by = y + 2;
        headerSprite.drawPixel(bx + 1, by,     _currentThemeColors->text_primary);
        headerSprite.drawPixel(bx,     by + 1, _currentThemeColors->text_primary);
        headerSprite.drawPixel(bx + 1, by + 2, _currentThemeColors->text_primary);
        headerSprite.drawPixel(bx,     by + 3, _currentThemeColors->text_primary);
        headerSprite.drawPixel(bx + 1, by + 4, _currentThemeColors->text_primary);
        headerSprite.drawPixel(bx,     by + 5, _currentThemeColors->text_primary);
        headerSprite.drawPixel(bx + 1, by + 6, _currentThemeColors->text_primary);
    }
}

void DisplayManager::drawTotpContainer() {
    int cornerRadius = 8;
    int containerW = totpContainerSprite.width() - 2;
    int containerH = totpContainerSprite.height() - 2;

    // 1. Очищаем спрайт контейнера
    totpContainerSprite.fillSprite(_currentThemeColors->background_dark);

    // 2. Рисуем тень со смещением
    totpContainerSprite.fillRoundRect(2, 2, containerW, containerH, cornerRadius, _currentThemeColors->shadow_color);
    
    // 3. Рисуем основной фон контейнера
    totpContainerSprite.fillRoundRect(0, 0, containerW, containerH, cornerRadius, _currentThemeColors->background_light);
    
    // 4. Рисуем обводку поверх фона
    totpContainerSprite.drawRoundRect(0, 0, containerW, containerH, cornerRadius, _currentThemeColors->text_secondary);
    
    _totpContainerNeedsRedraw = false;
    _lastDrawnTotpString = ""; 
}

void DisplayManager::drawTotpText(const String& textToDraw) {
    if (textToDraw == _lastDrawnTotpString && !_totpContainerNeedsRedraw) {
        return; 
    }

    // 1. Рисуем анимированный текст в свой спрайт
    totpSprite.fillSprite(_currentThemeColors->background_light);
    totpSprite.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_light);
    
    // Уменьшаем размер шрифта для длинного текста (например "NOT SYNCED")
    int textSize;
    if (textToDraw.length() <= 6) {
        textSize = 4;        // 6 digits - large
    } else if (textToDraw.length() <= 8) {
        textSize = 3;        // 8 digits - medium
    } else {
        textSize = 2;        // "NOT SYNCED" and other status text - same as before
    }
    totpSprite.setTextSize(textSize);
    totpSprite.drawString(textToDraw, totpSprite.width() / 2, totpSprite.height() / 2);

    // 2. Накладываем спрайт с текстом внутрь рамки контейнера со смещением в 1px
    totpSprite.pushToSprite(&totpContainerSprite, 1, 1);

    // 3. Выводим финальный спрайт контейнера на экран
    int centerX = tft.width() / 2;
    int codeY = tft.height() / 2;
    int containerX = centerX - totpContainerSprite.width() / 2;
    int containerY = codeY - totpContainerSprite.height() / 2;
    totpContainerSprite.pushSprite(containerX, containerY);

    _lastDrawnTotpString = textToDraw;
}


void DisplayManager::updateTOTPCode(const String& code, int timeRemaining, int period) {
    // 🚫 Не обновляем TOTP когда показывается QR код
    if (_qrCodeActive) {
        return;
    }
    
    if (_totpContainerNeedsRedraw) {
        drawTotpContainer();
    }

    // Если ключ был только что переключен, просто обновляем код без анимации
    if (_isKeySwitched) {
        _currentCode = code;
        _newCode = code;
        _totpState = TotpState::IDLE;
        _isKeySwitched = false; // Сбрасываем флаг
    }

    // Логика запуска анимации, если код изменился и это не переключение ключа
    if (code != _currentCode && _totpState == TotpState::IDLE) {
        if (_currentCode.length() > 0) {
            _newCode = code;
            _totpState = TotpState::SCRAMBLING;
            _totpAnimStartTime = millis();
            _lastScrambleFrameTime = millis();
        } else {
            _currentCode = code; // Первый код устанавливается без анимации
        }
    }
    
    // Если не анимируемся, просто рисуем текущий код
    if (_totpState == TotpState::IDLE) {
        drawTotpText(_currentCode);
    }

    // Обновление прогресс-бара времени
    if (timeRemaining != lastTimeRemaining) {
        int barY = tft.height() - 30;
        int barHeight = 10;
        int barWidth = (tft.width() - 64) * 0.8;
        int barX = (tft.width() - barWidth) / 2;
        int shadowOffset = 2;
        int barCornerRadius = 5;

        // Очищаем область прогресс-бара
        tft.fillRect(barX - shadowOffset, barY - shadowOffset, barWidth + 40 + shadowOffset, barHeight + shadowOffset * 2, _currentThemeColors->background_dark);
        
        // Check if this is HOTP mode (timeRemaining == -1)
        if (timeRemaining == -1) {
            // HOTP: Draw refresh icon instead of progress bar
            tft.setTextColor(_currentThemeColors->text_secondary, _currentThemeColors->background_dark);
            tft.setTextSize(2);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("HOTP - Static Code", tft.width() / 2, barY + barHeight / 2);
        } else {
            // TOTP: Draw progress bar and timer
            // Рисуем рамку и фон
            tft.fillRoundRect(barX + shadowOffset, barY + shadowOffset, barWidth, barHeight, barCornerRadius, _currentThemeColors->shadow_color);
            tft.drawRoundRect(barX, barY, barWidth, barHeight, barCornerRadius, _currentThemeColors->text_secondary);
            tft.fillRoundRect(barX, barY, barWidth, barHeight, barCornerRadius, _currentThemeColors->background_light);

            // Рисуем заполнение
            int fillWidth = map(timeRemaining, period, 0, barWidth, 0);
            tft.fillRoundRect(barX, barY, fillWidth, barHeight, barCornerRadius, _currentThemeColors->accent_primary);

            // Рисуем текст времени
            tft.setTextColor(_currentThemeColors->text_secondary, _currentThemeColors->background_dark);
            tft.setTextSize(2);
            tft.drawString(String(timeRemaining) + "s", barX + barWidth + 20, barY + barHeight / 2);
        }
        
        lastTimeRemaining = timeRemaining;
    }
}

void DisplayManager::showMessage(const String& text, int x, int y, bool isError, int size) {
    tft.setTextDatum(TL_DATUM);
    tft.setCursor(x, y);
    tft.setTextSize(size);
    tft.setTextColor(isError ? _currentThemeColors->error_color : _currentThemeColors->text_primary, _currentThemeColors->background_dark);
    tft.println(text);
    tft.setTextDatum(MC_DATUM);
}

void DisplayManager::showMessage(const String& text, int x, int y, bool isError, int size, bool inverted) {
    tft.setTextDatum(TL_DATUM);
    tft.setCursor(x, y);
    tft.setTextSize(size);
    if (inverted) {
        tft.setTextColor(_currentThemeColors->background_dark, _currentThemeColors->text_primary);
    } else {
        tft.setTextColor(isError ? _currentThemeColors->error_color : _currentThemeColors->text_primary, _currentThemeColors->background_dark);
    }
    tft.println(text);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_dark);
}

void DisplayManager::turnOff() {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWrite(TFT_BL, 0);
#else
  ledcWrite(0, 0);
#endif
  tft.writecommand(0x10); // TFT_SLPIN — stops internal oscillator (~5mA saved)
}

void DisplayManager::turnOn() {
  tft.writecommand(0x11); // TFT_SLPOUT — restart internal oscillator
  delay(120);             // ST7789 requires 120ms after SLPOUT before commands
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWrite(TFT_BL, 255);
#else
  ledcWrite(0, 255);
#endif
}
void DisplayManager::setBrightness(uint8_t brightness) {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWrite(TFT_BL, brightness);
#else
  ledcWrite(0, brightness);
#endif
}

// 🔄 Обновление текста без полной перерисовки экрана
void DisplayManager::updateMessage(const String& text, int x, int y, int size) {
    tft.setTextSize(size);
    tft.setTextDatum(TL_DATUM);
    
    // Рассчитываем размер области текста
    int textWidth = tft.textWidth(text);
    int textHeight = tft.fontHeight() * size;
    
    // Очищаем только область текста (с небольшим запасом)
    tft.fillRect(x, y, textWidth + 10, textHeight + 5, _currentThemeColors->background_dark);
    
    // Рисуем новый текст
    tft.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_dark);
    tft.drawString(text, x, y);
    
    tft.setTextDatum(MC_DATUM);
}

// 🧽 Очистка конкретной области
void DisplayManager::clearMessageArea(int x, int y, int width, int height) {
    tft.fillRect(x, y, width, height, _currentThemeColors->background_dark);
}

TFT_eSPI* DisplayManager::getTft() { return &tft; }
void DisplayManager::fillRect(int32_t x, int32_t t, int32_t w, int32_t h, uint32_t color) { tft.fillRect(x, t, w, h, color); }

bool DisplayManager::promptWebServerSelection() {
    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextColor(_currentThemeColors->text_primary);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Start Web Server?", tft.width() / 2, 30);

    bool selection = true; // true for "Yes", false for "No"
    
    auto drawButtons = [&](bool currentSelection) {
        int btnWidth = 80;
        int btnHeight = 40;
        int btnY = tft.height() / 2 + 10;
        int yesX = tft.width() / 2 - btnWidth - 10;
        int noX = tft.width() / 2 + 10;

        // Очищаем область кнопок перед перерисовкой
        tft.fillRect(yesX - 2, btnY - 2, btnWidth + 4, btnHeight + 4, _currentThemeColors->background_dark);
        tft.fillRect(noX - 2, btnY - 2, btnWidth + 4, btnHeight + 4, _currentThemeColors->background_dark);

        // Draw "Yes" button
        if (currentSelection) {
            tft.fillRoundRect(yesX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->accent_primary);
            tft.setTextColor(_currentThemeColors->background_dark);
        } else {
            tft.fillRoundRect(yesX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->background_dark);
            tft.drawRoundRect(yesX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->text_secondary);
            tft.setTextColor(_currentThemeColors->text_primary);
        }
        tft.drawString("Yes", yesX + btnWidth / 2, btnY + btnHeight / 2);

        // Draw "No" button
        if (!currentSelection) {
            tft.fillRoundRect(noX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->accent_primary);
            tft.setTextColor(_currentThemeColors->background_dark);
        } else {
            tft.fillRoundRect(noX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->background_dark);
            tft.drawRoundRect(noX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->text_secondary);
            tft.setTextColor(_currentThemeColors->text_primary);
        }
        tft.drawString("No", noX + btnWidth / 2, btnY + btnHeight / 2);
        
        // Reset text color to default for other text elements
        tft.setTextColor(_currentThemeColors->text_primary);
    };

    drawButtons(selection);

    unsigned long startTime = millis();
    const unsigned long timeout = 2000; // 2 секунды таймаут

    while (millis() - startTime < timeout) {
        // Button 1 (top, GPIO 35) to toggle
        if (digitalRead(BUTTON_1) == LOW) {
            selection = !selection;
            drawButtons(selection);
            delay(300); // Debounce
            startTime = millis(); // Reset timeout on activity
        }

        // Button 2 (bottom, GPIO 0) to confirm
        if (digitalRead(BUTTON_2) == LOW) {
            delay(300); // Debounce
            
            // 🧹 Очистка экрана перед возвратом
            tft.fillScreen(_currentThemeColors->background_dark);
            delay(50); // Даем дисплею время на обновление
            
            return selection;
        }
        delay(50);
    }

    // 🧹 КРИТИЧНО: Очистка экрана после таймаута
    // Без этого кнопки промптинга остаются на экране!
    tft.fillScreen(_currentThemeColors->background_dark);
    
    return false; // Default to "No" after timeout
}

// 🌌 Промптинг выбора режима запуска (AP/Offline/WiFi)
StartupMode DisplayManager::promptModeSelection(StartupMode defaultMode) {
    // Определяем две кнопки — режимы, которые НЕ являются дефолтом
    StartupMode leftMode, rightMode;
    if (defaultMode == StartupMode::WIFI_MODE) {
        leftMode = StartupMode::AP_MODE;
        rightMode = StartupMode::OFFLINE_MODE;
    } else if (defaultMode == StartupMode::AP_MODE) {
        leftMode = StartupMode::WIFI_MODE;
        rightMode = StartupMode::OFFLINE_MODE;
    } else {
        leftMode = StartupMode::WIFI_MODE;
        rightMode = StartupMode::AP_MODE;
    }

    auto modeLabel = [](StartupMode m) -> const char* {
        if (m == StartupMode::AP_MODE) return "AP";
        if (m == StartupMode::OFFLINE_MODE) return "Offline";
        return "WiFi";
    };

    auto defaultSubtitle = [](StartupMode m) -> const char* {
        if (m == StartupMode::AP_MODE) return "Auto: AP Mode (default)";
        if (m == StartupMode::OFFLINE_MODE) return "Auto: Offline Mode (default)";
        return "Auto: WiFi Mode (default)";
    };

    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextColor(_currentThemeColors->text_primary);
    tft.setTextDatum(MC_DATUM);
    
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    const int modeYOff = (tft.height() / 2 - 55) / 2;
#else
    const int modeYOff = 0;
#endif
    
    tft.setTextSize(2);
    tft.drawString("Select Mode", tft.width() / 2, 20 + modeYOff);
    
    // Подзаголовок с подсказкой — динамически показывает дефолтный режим
    tft.setTextSize(1);
    tft.setTextColor(_currentThemeColors->text_secondary);
    tft.drawString(defaultSubtitle(defaultMode), tft.width() / 2, 45 + modeYOff);

    bool selection = true; // true = левая кнопка (leftMode), false = правая (rightMode)
    
    auto drawButtons = [&](bool currentSelection) {
        int btnWidth = 80;
        int btnHeight = 40;
        int btnY = tft.height() / 2 + 10;
        int leftX = tft.width() / 2 - btnWidth - 10;
        int rightX = tft.width() / 2 + 10;

        // Очистка области кнопок
        tft.fillRect(leftX - 2, btnY - 2, btnWidth + 4, btnHeight + 4, _currentThemeColors->background_dark);
        tft.fillRect(rightX - 2, btnY - 2, btnWidth + 4, btnHeight + 4, _currentThemeColors->background_dark);

        // 🔘 Левая кнопка
        if (currentSelection) {
            tft.fillRoundRect(leftX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->accent_primary);
            tft.setTextColor(_currentThemeColors->background_dark);
        } else {
            tft.fillRoundRect(leftX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->background_dark);
            tft.drawRoundRect(leftX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->text_secondary);
            tft.setTextColor(_currentThemeColors->text_secondary);
        }
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(leftMode == StartupMode::OFFLINE_MODE ? 1 : 2);
        tft.drawString(modeLabel(leftMode), leftX + btnWidth/2, btnY + btnHeight/2);

        // 🔘 Правая кнопка
        if (!currentSelection) {
            tft.fillRoundRect(rightX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->accent_primary);
            tft.setTextColor(_currentThemeColors->background_dark);
        } else {
            tft.fillRoundRect(rightX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->background_dark);
            tft.drawRoundRect(rightX, btnY, btnWidth, btnHeight, 8, _currentThemeColors->text_secondary);
            tft.setTextColor(_currentThemeColors->text_secondary);
        }
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(rightMode == StartupMode::OFFLINE_MODE ? 1 : 2);
        tft.drawString(modeLabel(rightMode), rightX + btnWidth/2, btnY + btnHeight/2);
        
        // Сброс цвета текста
        tft.setTextColor(_currentThemeColors->text_primary);
    };

    drawButtons(selection);

    unsigned long startTime = millis();
    const unsigned long timeout = 2000; // 2 секунды таймаут

    while (millis() - startTime < timeout) {
        // Button 1 (GPIO 35) - переключение между двумя не-дефолтными режимами
        if (digitalRead(BUTTON_1) == LOW) {
            selection = !selection;
            drawButtons(selection);
            delay(300); // Debounce
            startTime = millis(); // Сброс таймаута при активности
        }

        // Button 2 (GPIO 0) - подтверждение выбора
        if (digitalRead(BUTTON_2) == LOW) {
            delay(300); // Debounce
            
            // 🧹 Очистка экрана перед переходом к выбранному режиму
            tft.fillScreen(_currentThemeColors->background_dark);
            delay(50);
            
            return selection ? leftMode : rightMode;
        }
        delay(50);
    }

    // 🧹 Очистка экрана после таймаута
    tft.fillScreen(_currentThemeColors->background_dark);
    
    return defaultMode; // По умолчанию — выбранный пользователем boot mode
}

// Draw N lock icons at (x, y). Color encodes strength level.
void DisplayManager::drawLockIcons(int x, int y, uint8_t strength) {
    // Colors: 1=red(weak), 2=yellow(medium), 3=white(strong)
    uint16_t colors[3];
    if (strength == 3) {
        colors[0] = colors[1] = colors[2] = _currentThemeColors->accent_secondary;
    } else if (strength == 2) {
        colors[0] = colors[1] = _currentThemeColors->accent_secondary;
        colors[2] = _currentThemeColors->text_secondary;
    } else {
        colors[0] = _currentThemeColors->error_color;
        colors[1] = _currentThemeColors->text_secondary;
        colors[2] = _currentThemeColors->text_secondary;
    }
    
    uint8_t count = (strength == 0) ? 0 : strength;
    for (int i = 0; i < 3; i++) {
        int lx = x + i * 10;
        uint16_t col = (i < count) ? colors[i] : _currentThemeColors->text_secondary;
        
        // Lock body (6x5)
        tft.fillRoundRect(lx, y + 4, 6, 5, 1, col);
        
        // Lock shackle outline (4x4), open at bottom
        tft.drawRect(lx + 1, y, 4, 4, col);
        
        // Clear bottom of shackle where it meets body
        tft.fillRect(lx + 1, y + 3, 4, 2, _currentThemeColors->background_dark);
    }
}

// Generic text badge at (x, y). w=30, h=12.
void DisplayManager::drawTextBadge(int x, int y, const char* label, uint16_t bg) {
    uint16_t fg = 0x0000; // black text
    tft.fillRoundRect(x, y, 30, 12, 2, bg);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(fg, bg);
    tft.drawString(label, x + 15, y + 6);
}

// Draw DUP badge at (x, y). Width ~30px, height 12px.
void DisplayManager::drawDupBadge(int x, int y) {
    uint16_t bg  = 0xC480; // muted amber — visible but not alarming
    uint16_t fg  = 0x0000; // black text on amber
    tft.fillRoundRect(x, y, 30, 12, 2, bg);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(fg, bg);
    tft.drawString("DUP", x + 15, y + 6);
}

void DisplayManager::drawPasswordLayout(const String& name, const String& password,
                                        int batteryPercentage, bool isCharging,
                                        bool isWebServerOn,
                                        uint8_t strength, bool isDuplicate,
                                        bool isPin, bool isName) {
    if (_isNoItemsPageActive) {
        _isNoItemsPageActive = false;
        tft.fillScreen(_currentThemeColors->background_dark); 
    } else {
        // Очищаем только область контента под заголовком, чтобы избежать мерцания
        tft.fillRect(0, headerSprite.height(), tft.width(), tft.height() - headerSprite.height(), _currentThemeColors->background_dark);
    }
    
    _currentServiceName = name; // Используем ту же переменную для имени
    _currentBatteryPercentage = batteryPercentage;
    _isCharging = isCharging;
    _isWebServerOn = isWebServerOn;
    _headerState = HeaderState::INTRO;
    _introAnimStartTime = millis();

    // Отрисовка частично замаскированного пароля
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_dark);
    tft.setTextSize(2); // Меньший размер шрифта
    
    // Создаем частично замаскированную версию пароля
    String maskedPassword = "";
    if (password.length() == 0) {
        maskedPassword = "[Empty]";
    } else if (password.length() == 1) {
        maskedPassword = password.substring(0, 1) + "*";
    } else if (password.length() == 2) {
        maskedPassword = password;
    } else {
        // Показываем первые 2 символа + звездочки для остальных
        maskedPassword = password.substring(0, 2);
        int remainingChars = password.length() - 2;
        for (int i = 0; i < remainingChars && i < 10; i++) { // Ограничиваем количество звездочек
            maskedPassword += "*";
        }
        if (remainingChars > 10) {
            maskedPassword += "...";
        }
    }
    
    tft.drawString(maskedPassword, tft.width() / 2, tft.height() / 2);

    // --- Lower quarter: centered badge row ---
    {
        const int lockW  = 30;
        const int badgeW = 30;
        const int gap    = 6;
        int rowY = tft.height() * 3 / 4;
        
        // Count active elements to compute total width
        int elems = 0;
        if (strength > 0) elems++;
        if (isDuplicate)  elems++;
        if (isPin)        elems++;
        if (isName)       elems++;
        
        int totalW = 0;
        if (strength > 0) totalW += lockW;
        int badgeCount = (isDuplicate?1:0) + (isPin?1:0) + (isName?1:0);
        totalW += badgeCount * badgeW;
        if (elems > 1) totalW += (elems - 1) * gap;
        
        int curX = (tft.width() - totalW) / 2;
        
        if (strength > 0) {
            drawLockIcons(curX, rowY - 5, strength);
            curX += lockW + gap;
        }
        if (isDuplicate) {
            drawDupBadge(curX, rowY - 6);
            curX += badgeW + gap;
        }
        if (isPin) {
            drawTextBadge(curX, rowY - 6, "PIN", 0xF800); // red
            curX += badgeW + gap;
        }
        if (isName) {
            drawTextBadge(curX, rowY - 6, "NAME", 0xFD20); // orange
        }
    }

    lastDisplayedCode = ""; 
    lastTimeRemaining = -1;
    _lastDrawnTotpString = "";
    _totpState = TotpState::IDLE;
    _totpContainerNeedsRedraw = true;
}

void DisplayManager::drawBleInitLoader(int progress) {
    drawGenericLoader(progress, HID_LOADER_TEXT);
}

#ifdef BOARD_HAS_USB_HID
void DisplayManager::drawUsbHidPage(const String& passwordName, const String& status) {
    // Full redraw every time — same as drawBleConfirmPage, no caching
    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextDatum(MC_DATUM);
    
    // Password name at top — same as BLE confirm
    tft.setTextSize(1);
    tft.setTextColor(_currentThemeColors->text_secondary);
    String displayName = passwordName.length() > 20
        ? passwordName.substring(0, 20) + "..." : passwordName;
    tft.drawString(displayName, tft.width() / 2, 15);
    
    // Center label
    tft.setTextSize(2);
    tft.setTextColor(_currentThemeColors->text_primary);
    tft.drawString("USB HID", tft.width() / 2, tft.height() / 2 - 5);
    
    // Status line — same position as "BLE Connected"
    bool isReady = (status == "USB Connected");
    tft.setTextSize(1);
    tft.setTextColor(isReady ? _currentThemeColors->accent_primary
                              : _currentThemeColors->text_secondary);
    tft.drawString(status, tft.width() / 2, tft.height() / 2 + 20);
    
    // Button hints — same as BLE confirm
    tft.setTextSize(1);
    tft.setTextColor(_currentThemeColors->text_secondary);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Back", 5, tft.height() - 10);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("Send", tft.width() - 5, tft.height() - 10);
    tft.setTextDatum(MC_DATUM);
    
    _usbHidLastStatus = status;
}

void DisplayManager::resetUsbHidPage() {
    _usbHidLastStatus = "";
}

void DisplayManager::setUsbHidMode(bool active) {
    _usbHidMode = active;
}

bool DisplayManager::drawHidPrompt(bool defaultIsBle) {
    // Ждём пока кнопки отпустят после удержания
    unsigned long releaseWait = millis();
    while ((digitalRead(BUTTON_1) == LOW || digitalRead(BUTTON_2) == LOW)
           && millis() - releaseWait < 1000) {
        delay(10);
    }
    delay(50); // доп. debounce
    
    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextDatum(MC_DATUM);
    
    // Заголовок — что будет по дефолту
    tft.setTextColor(_currentThemeColors->text_primary);
    tft.setTextSize(2);
    tft.drawString(defaultIsBle ? "BLE Send" : "USB Send", tft.width() / 2, tft.height() / 2 - 30);
    
    // Подсказка — как переключить
    tft.setTextSize(1);
    tft.setTextColor(_currentThemeColors->text_secondary);
    tft.drawString(defaultIsBle ? "Hold BTN2 for USB" : "Hold BTN2 for BLE",
                   tft.width() / 2, tft.height() / 2 + 5);
    
    // Нижняя подпись
    tft.setTextColor(_currentThemeColors->text_secondary);
    tft.drawString("Auto in 3s...", tft.width() / 2, tft.height() / 2 + 25);
    
    unsigned long startTime = millis();
    const unsigned long timeout = 3000;
    
    while (millis() - startTime < timeout) {
        // BTN2 — переключить на BLE (если дефолт USB)
        if (!defaultIsBle && digitalRead(BUTTON_2) == LOW) {
            delay(50);
            if (digitalRead(BUTTON_2) == LOW) { delay(200); return true; }
        }
        // BTN2 — переключить на USB (если дефолт BLE)
        if (defaultIsBle && digitalRead(BUTTON_2) == LOW) {
            delay(50);
            if (digitalRead(BUTTON_2) == LOW) { delay(200); return false; }
        }
        delay(10);
    }
    
    return defaultIsBle;
}
#endif

void DisplayManager::drawHOTPLoader(int progress) {
    if (_qrCodeActive) return;
    
    // Оптимизированная перерисовка - перерисовываем только при изменениях
    bool needsFullRedraw = !_loaderActive || _lastLoaderText != "HOTP";
    bool needsProgressUpdate = _lastLoaderProgress != progress;
    
    int barY = tft.height() - 30;
    int barHeight = 10;
    int barWidth = (tft.width() - 64) * 0.8;
    int barX = (tft.width() - barWidth) / 2;
    int shadowOffset = 2;
    int barCornerRadius = 5;

    if (needsFullRedraw) {
        // Clear full width to erase "HOTP - Static Code" text
        tft.fillRect(0, barY - 10, tft.width(), 40, _currentThemeColors->background_dark);

        // Draw shadow, border, background
        tft.fillRoundRect(barX + shadowOffset, barY + shadowOffset, barWidth, barHeight, barCornerRadius, _currentThemeColors->shadow_color);
        tft.drawRoundRect(barX, barY, barWidth, barHeight, barCornerRadius, _currentThemeColors->text_secondary);
        tft.fillRoundRect(barX, barY, barWidth, barHeight, barCornerRadius, _currentThemeColors->background_light);

        _lastLoaderText = "HOTP";
        _loaderActive = true;
    }
    
    if (needsProgressUpdate) {
        // Clear interior before redrawing fill
        tft.fillRoundRect(barX + 1, barY + 1, barWidth - 2, barHeight - 2, barCornerRadius, _currentThemeColors->background_light);
        
        // Draw fill based on progress (0-100)
        int fillWidth = map(progress, 0, 100, 0, barWidth);
        tft.fillRoundRect(barX, barY, fillWidth, barHeight, barCornerRadius, _currentThemeColors->accent_primary);
        
        _lastLoaderProgress = progress;
    }
}

void DisplayManager::drawGenericLoader(int progress, const String& text) {
    // Оптимизированная перерисовка - перерисовываем только при изменениях
    bool needsFullRedraw = !_loaderActive || _lastLoaderText != text;
    bool needsProgressUpdate = _lastLoaderProgress != progress;
    
    if (needsFullRedraw) {
        // Полная перерисовка с фоном согласно теме для предотвращения миганий
        tft.fillScreen(_currentThemeColors->background_dark);
        
        // Рисуем основные элементы с цветами темы
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(_currentThemeColors->text_primary);
        tft.setTextSize(2);
        
        // Заголовок
        int textY = tft.height() / 2 - 30;
        tft.drawString(text, tft.width() / 2, textY);
        
        // Рамка прогресс-бара (один раз)
        int barWidth = 100;
        int barHeight = 12;
        int barX = (tft.width() - barWidth) / 2;
        int barY = tft.height() / 2 + 5;
        
        // Фон под прогресс-баром согласно теме для предотвращения артефактов
        tft.fillRect(barX - 2, barY - 2, barWidth + 4, barHeight + 4, _currentThemeColors->background_dark);
        
        // Рамка прогресс-бара с цветом темы
        tft.drawRect(barX, barY, barWidth, barHeight, _currentThemeColors->text_secondary);
        
        _lastLoaderText = text;
        _loaderActive = true;
    }
    
    if (needsProgressUpdate) {
        // Обновляем только заливку прогресс-бара
        int barWidth = 100;
        int barHeight = 12;
        int barX = (tft.width() - barWidth) / 2;
        int barY = tft.height() / 2 + 5;
        
        // Очищаем внутренность бара фоном темы
        tft.fillRect(barX + 1, barY + 1, barWidth - 2, barHeight - 2, _currentThemeColors->background_dark);
        
        // Рисуем новую заливку с акцентным цветом темы
        int fillWidth = (barWidth - 2) * progress / 100;
        if (fillWidth > 0) {
            tft.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, _currentThemeColors->accent_primary);
        }
        
        _lastLoaderProgress = progress;
    }
}

void DisplayManager::hideLoader() {
    if (!_loaderActive) return; // Не делаем ничего, если лоадер не был активен
    _loaderActive = false;
    _lastLoaderText = "";
    _lastLoaderProgress = -1;
    _isNoItemsPageActive = false; // Сбрасываем флаг, чтобы вызвать полную перерисовку
    tft.fillScreen(_currentThemeColors->background_dark); // Очищаем экран
}

void DisplayManager::resetLoaderState() {
    _loaderActive = false;
    _lastLoaderText = "";
    _lastLoaderProgress = -1;
}

void DisplayManager::eraseLoaderArea() {
    _loaderActive = false;
    _lastLoaderText = "";
    _lastLoaderProgress = -1;
    lastTimeRemaining = -999; // Force redraw of HOTP/TOTP area on next updateTOTPCode call
    // Clear only the loader bar area at bottom of screen
    int barY = tft.height() - 30;
    tft.fillRect(0, barY - 10, tft.width(), 40, _currentThemeColors->background_dark);
}

void DisplayManager::drawBleAdvertisingPage(const String& deviceName, const String& status, int timeLeft) {
    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(_currentThemeColors->text_primary);
    
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    const int yOff = (tft.height() - 120) / 2;
#else
    const int yOff = 0;
#endif
    
    tft.setTextSize(2);
    tft.drawString("BLE Broadcasting", tft.width() / 2, 20 + yOff);
    
    tft.setTextSize(1);
    tft.drawString("Device Name:", tft.width() / 2, 50 + yOff);
    tft.setTextSize(2);
    tft.drawString(deviceName, tft.width() / 2, 70 + yOff);
    
    tft.setTextSize(1);
    tft.drawString(status, tft.width() / 2, 100 + yOff);

    // Button labels
    tft.setTextSize(1);
    tft.drawString("Back", 30, tft.height() - 20);
}

void DisplayManager::drawBleConfirmPage(const String& passwordName, const String& password, const String& deviceName) {
    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(_currentThemeColors->text_primary);
    
    // Имя пароля сверху
    tft.setTextSize(1);
    tft.setTextColor(_currentThemeColors->text_secondary);
    String displayName = passwordName;
    if (displayName.length() > 20) {
        displayName = displayName.substring(0, 20) + "...";
    }
    tft.drawString(displayName, tft.width() / 2, 15);
    
    // Замаскированный пароль в центре
    tft.setTextSize(2);
    tft.setTextColor(_currentThemeColors->text_primary);
    String maskedPassword = "";
    for (int i = 0; i < password.length() && i < 12; i++) {
        maskedPassword += "*";
    }
    if (password.length() > 12) {
        maskedPassword += "...";
    }
    tft.drawString(maskedPassword, tft.width() / 2, tft.height() / 2 - 5);
    
    // Информация об устройстве
    tft.setTextSize(1);
    tft.setTextColor(_currentThemeColors->accent_primary);
    tft.drawString("BLE Connected", tft.width() / 2, tft.height() / 2 + 20);

    // Кнопки внизу
    tft.setTextSize(1);
    tft.setTextColor(_currentThemeColors->text_secondary);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Back", 5, tft.height() - 10);
    
    tft.setTextDatum(TR_DATUM);
    tft.drawString("Send", tft.width() - 5, tft.height() - 10);
    
    // Восстанавливаем центральное выравнивание
    tft.setTextDatum(MC_DATUM);
}

void DisplayManager::drawBleSendingPage() {
    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(_currentThemeColors->text_primary);
    tft.setTextSize(2);
    tft.drawString("Sending...", tft.width() / 2, tft.height() / 2);
}

void DisplayManager::drawBleResultPage(bool success) {
    tft.fillScreen(_currentThemeColors->background_dark);
    tft.setTextDatum(MC_DATUM);
    if (success) {
        tft.setTextColor(_currentThemeColors->accent_primary);
        tft.setTextSize(2);
        tft.drawString("Sent Successfully!", tft.width() / 2, tft.height() / 2);
    } else {
        tft.setTextColor(_currentThemeColors->error_color);
        tft.setTextSize(2);
        tft.drawString("Failed to Send", tft.width() / 2, tft.height() / 2);
    }
}

void DisplayManager::drawNoItemsPage(const String& text) {
    if (_isNoItemsPageActive) {
        // Если страница уже отображается, ничего не делаем, чтобы избежать мерцания.
        // Обновление батареи будет происходить отдельно в main loop.
        return;
    }

    _isNoItemsPageActive = true;
    tft.fillScreen(_currentThemeColors->background_dark);
    
    // 🔋 Рисуем индикатор батареи в правом верхнем углу
    int batteryX = tft.width() - 28;
    int batteryY = 5;
    int batteryWidth = 22;
    int batteryHeight = 12;
    int batteryCornerRadius = 3;
    int shadowOffset = 1;
    
    // Тень батареи
    tft.drawRoundRect(batteryX + shadowOffset, batteryY + shadowOffset, batteryWidth, batteryHeight, batteryCornerRadius, _currentThemeColors->shadow_color);
    tft.fillRect(batteryX + batteryWidth + shadowOffset, batteryY + 3 + shadowOffset, 2, 5, _currentThemeColors->shadow_color);
    
    // Обводка батареи
    tft.drawRoundRect(batteryX, batteryY, batteryWidth, batteryHeight, batteryCornerRadius, _currentThemeColors->text_secondary);
    tft.fillRect(batteryX + batteryWidth, batteryY + 3, 2, 5, _currentThemeColors->text_secondary);
    
    // Заполнение батареи
    uint16_t barColor;
    int barWidth;
    if (_currentBatteryPercentage >= 20) {
        barColor = _currentThemeColors->accent_primary;
    } else {
        barColor = _currentThemeColors->error_color;
    }
    barWidth = map(_currentBatteryPercentage, 0, 100, 0, batteryWidth - 4);
    if (barWidth > 0) {
        tft.fillRect(batteryX + 2, batteryY + 2, barWidth, batteryHeight - 4, barColor);
    }
    
    // 🕐 Часы в левом верхнем углу (прямо в tft, не спрайт)
    {
        struct tm timeinfo;
        bool synced = getLocalTime(&timeinfo, 0);
        uint16_t col = _currentThemeColors->text_secondary;
        uint16_t bg  = _currentThemeColors->background_dark;
        int cy = 4;

        auto drawDigitTft = [&](int dx, int dy, int d) {
            tft.fillRect(dx, dy, 5, 9, bg);
            if (d < 0 || d > 9) {
                tft.drawFastHLine(dx+1, dy+4, 3, col);
                return;
            }
            if (d==0||d==2||d==3||d==5||d==6||d==7||d==8||d==9)
                tft.drawFastHLine(dx+1, dy,   3, col);
            if (d==0||d==1||d==2||d==3||d==4||d==7||d==8||d==9)
                tft.drawFastVLine(dx+4, dy+1, 3, col);
            if (d==0||d==1||d==3||d==4||d==5||d==6||d==7||d==8||d==9)
                tft.drawFastVLine(dx+4, dy+5, 3, col);
            if (d==0||d==2||d==3||d==5||d==6||d==8||d==9)
                tft.drawFastHLine(dx+1, dy+8, 3, col);
            if (d==0||d==2||d==6||d==8)
                tft.drawFastVLine(dx,   dy+5, 3, col);
            if (d==0||d==4||d==5||d==6||d==8||d==9)
                tft.drawFastVLine(dx,   dy+1, 3, col);
            if (d==2||d==3||d==4||d==5||d==6||d==8||d==9)
                tft.drawFastHLine(dx+1, dy+4, 3, col);
        };

        if (!synced) {
            drawDigitTft(4,  cy, -1);
            drawDigitTft(11, cy, -1);
            drawDigitTft(21, cy, -1);
            drawDigitTft(28, cy, -1);
        } else {
            int h = timeinfo.tm_hour;
            int m = timeinfo.tm_min;
            drawDigitTft(4,  cy, h / 10);
            drawDigitTft(11, cy, h % 10);
            drawDigitTft(21, cy, m / 10);
            drawDigitTft(28, cy, m % 10);
        }
        tft.drawPixel(18, cy+2, col);
        tft.drawPixel(18, cy+6, col);
    }
    
    // 📏 Размеры округленной рамки (в области TOTP кода)
    int boxWidth = 180;
    int boxHeight = 70;
    int boxX = (tft.width() - boxWidth) / 2;
    int boxY = tft.height() / 2 - boxHeight / 2;
    int cornerRadius = 12;
    
    // 🔲 Рисуем округленную рамку
    tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, cornerRadius, _currentThemeColors->text_secondary);
    
    // 📝 Текст внутри рамки (меньше размер, отцентрован)
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(_currentThemeColors->text_primary);
    tft.setTextSize(1); // Уменьшенный размер
    
    String line1 = "No " + text + " found";
    tft.drawString(line1, tft.width() / 2, tft.height() / 2 - 12);
    
    tft.setTextColor(_currentThemeColors->text_secondary);
    tft.drawString("Add via Web UI", tft.width() / 2, tft.height() / 2 + 8);
}


// ==================== QR CODE DISPLAY ====================

void DisplayManager::showQRCode(const String& text, int timeoutSeconds) {
    LOG_INFO("DisplayManager", "Showing QR code for " + String(timeoutSeconds) + " seconds");
    LOG_DEBUG("DisplayManager", "QR text length: " + String(text.length()) + " chars");
    
    // Save current state
    _qrCodeActive = true;
    _qrCodeTimeout = millis() + (timeoutSeconds * 1000);
    
    // CRITICAL: Full screen clear
    tft.fillScreen(_currentThemeColors->background_dark);
    
    // Create QR code with QRcode_eSPI
    QRcode_eSPI qrcode(&tft);
    
    // Extract key name from TOTP URI
    String keyName = "";
    int nameStart = text.indexOf("totp/");
    if (nameStart != -1) {
        nameStart += 5;
        int nameEnd = text.indexOf("?", nameStart);
        if (nameEnd != -1) {
            keyName = text.substring(nameStart, nameEnd);
        }
    }
    
    // Header with key name (ABOVE QR code)
    if (keyName.length() > 0) {
        tft.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_dark);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        
        if (keyName.length() > 30) {
            keyName = keyName.substring(0, 27) + "...";
        }
        
        tft.drawString(keyName, 120, 10);
    }
    
    // Generate and display QR code
    // QRcode_eSPI automatically centers and scales
    qrcode.init();
    qrcode.create(text);
    
    // Timer at bottom of screen
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    _qrCodeTimerY = tft.height() - 20;
#else
    _qrCodeTimerY = 120;
#endif
    updateQRTimer(timeoutSeconds);
    
    LOG_INFO("DisplayManager", "QR code displayed");
}

void DisplayManager::updateQRTimer(int secondsRemaining) {
    // Clear only timer area
    int timerWidth = 40;
    int timerX = (tft.width() - timerWidth) / 2;
    tft.fillRect(timerX - 5, _qrCodeTimerY - 10, timerWidth + 10, 15, _currentThemeColors->background_dark);
    
    // Draw updated timer (centered)
    tft.setTextColor(_currentThemeColors->text_secondary, _currentThemeColors->background_dark);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(secondsRemaining) + "s", tft.width() / 2, _qrCodeTimerY);
}

void DisplayManager::hideQRCode() {
    if (_qrCodeActive) {
        LOG_INFO("DisplayManager", "Hiding QR code");
        _qrCodeActive = false;
        if (_isCharging) {
            _chargingAnimStartTime = millis();
        }
        _qrCodeTimeout = 0;
        _lastQRTimerSeconds = -1;
        
        // Clear screen and return to normal mode
        tft.fillScreen(_currentThemeColors->background_dark);
        _totpContainerNeedsRedraw = true;
    }
}

// Thread-safe request to show QR code (called from web server)
void DisplayManager::requestShowQRCode(const String& text, int timeoutSeconds) {
    _qrCodeText = text;
    _qrCodeTimeoutSeconds = timeoutSeconds;
    _qrCodeRequested = true;
    LOG_INFO("DisplayManager", "QR code show requested (will be displayed in main loop)");
}
