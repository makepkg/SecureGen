#ifndef SPLASH_MANAGER_H
#define SPLASH_MANAGER_H

#include "config.h"
#include "display_manager.h"

// SPLASH_IMAGE_PATH is defined in config.h
#define SPLASH_CONFIG_PATH "/splash_config.json"
// Embedded splash images are always 240x135 RGB565
#define SPLASH_IMAGE_WIDTH 240
#define SPLASH_IMAGE_HEIGHT 135

// 🎨 Модульная система для добавления новых splash screens
class SplashScreenManager {
public:
    SplashScreenManager(DisplayManager& displayManager);
    
    // Основной метод отображения
    void displaySplashScreen();
    
    // Управление конфигурацией (выбор встроенного splash)
    String loadSplashConfig();  // Возвращает: "disabled", "bladerunner", "combs", "securegen"
    bool saveSplashConfig(const String& mode);
    // 🔒 SECURITY: Custom splash upload/delete methods removed for security
    
private:
    DisplayManager& _displayManager;
    
    // Отображение встроенных splash screens
    void displayEmbeddedSplash(const String& mode);
};

#endif // SPLASH_MANAGER_H
