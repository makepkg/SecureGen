## USB Serial на S3 — как это работает

Да, кабель + кнопка Serial Monitor — это именно оно. Разница в том что на ESP32 оригинальном есть отдельный чип CH340 который всегда создаёт порт. На S3 порт создаётся самим ESP32 через нативный USB — и порт появляется **только после того как прошивка стартовала и инициализировала CDC**. Это занимает ~1.5 секунды. Если открыть монитор слишком рано — пропустишь первые логи. Решение: открывай монитор, потом нажимай RST на плате.

---

## Документация: масштабирование фич на две платы

### Правило одного файла

Всё что специфично для платы — только в `include/boards/`. Никаких `#ifdef` в бизнес-логике. Исключение только для hardware API (sleep, ADC, Serial) где платформа принципиально разная.

```
Бизнес-логика (main.cpp, managers)  →  никаких #ifdef платформы
Hardware abstraction (board_*.h)    →  все константы и пины
platformio.ini build_flags          →  все TFT/display параметры
```

---

### Три типа различий и как их решать

**Тип 1 — Пины и константы** (кнопки, ADC, I2C)

Решение: добавить константу в оба `board_*.h`. Код не трогать.

```cpp
// include/boards/board_esp32.h
#define MY_NEW_PIN 34

// include/boards/board_s3.h  
#define MY_NEW_PIN 5
```

**Тип 2 — Разные hardware API** (sleep, ADC calibration, PWM)

Решение: `#ifdef ARDUINO_LILYGO_T_DISPLAY_S3` только в одном месте, близко к железу.

```cpp
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    esp_sleep_enable_ext1_wakeup((1ULL << WAKEUP_GPIO), ESP_EXT1_WAKEUP_ANY_LOW);
#else
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);
#endif
esp_deep_sleep_start();
```

**Тип 3 — UI/Display** (координаты, размеры, layout)

Решение: макросы через `board_config.h`, никаких абсолютных чисел в display коде.

```cpp
// Неправильно:
tft.drawString("text", 120, 67);

// Правильно:
tft.drawString("text", TFT_WIDTH/2, TFT_HEIGHT/2);
```

---

### Как добавить новую фичу — чеклист

```
1. Есть ли hardware зависимость (пин, API)?
   Да → добавить в board_esp32.h и board_s3.h
   Нет → писать обычный код

2. Есть ли UI элементы с координатами?
   Да → использовать TFT_WIDTH/TFT_HEIGHT, не абсолютные числа

3. Есть ли разница в поведении платформ?
   Да → #ifdef ARDUINO_LILYGO_T_DISPLAY_S3 в одном месте

4. Компилировать оба таргета перед коммитом:
   pio run -e lilygo-t-display
   pio run -e lilygo-t-display-s3
```

---

### Барьеры масштабирования — что потребует работы

| Барьер | Когда столкнёшься | Решение |
|---|---|---|
| UI Layout | При портировании новых экранов | Заменить хардкодные координаты на макросы |
| PSRAM | При больших буферах (изображения, JSON) | `#ifdef BOARD_HAS_PSRAM` + `ps_malloc()` |
| BLE отличия | Если добавишь новые BLE профили | S3 BLE 5.0 vs ESP32 BLE 4.2 — API совместимы |
| ADC calibration | Если добавишь новые аналоговые датчики | S3 использует другой ADC channel mapping |
| ledc PWM API | Если добавишь новые PWM выходы | На S3 `ledcAttach()` вместо `ledcSetup()+ledcAttachPin()` |

---

### Золотое правило

Если пишешь `#ifdef` в `main.cpp` или в manager — это сигнал что нужно вынести абстракцию в `board_config.h` или создать wrapper функцию. Один `#ifdef` в одном месте лучше чем пять разбросанных по коду.