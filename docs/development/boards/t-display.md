## LilyGo T-Display ESP32 (1.14") — Справочная документация

### Чипсет и ресурсы

**MCU:** ESP32 (классический, двухъядерный Xtensa LX6)
**Wireless:** Wi-Fi 802.11 b/g/n + Bluetooth (Dual Mode)
**Voltage:** 3.3V логика

> ⚠️ **Отличие от T-Display-S3:** классический ESP32, не S3. Разная архитектура, разные GPIO-маппинги, другой USB-чип.

---

### Дисплей

| Параметр | Значение |
|---|---|
| Драйвер | ST7789V |
| Размер | 1.14" по диагонали |
| Тип | IPS TFT LCD |
| Интерфейс | **SPI** (в отличие от S3 с 8-bit parallel!) |

**GPIO дисплея (SPI):**

| Сигнал | GPIO |
|---|---|
| MOSI | 19 |
| SCLK | 18 |
| CS | 5 |
| DC | 16 |
| RST | 23 |
| BL (подсветка) | 4 |

---

### Pinout

**Левый ряд:**
| Пин | Функции |
|---|---|
| 21 | ADC4, SDA (I2C) |
| 22 | ADC4, SCL (I2C) |
| 17 | GPIO |
| 2 | TOUCH2, ADC12, GPIO |
| 13 | TOUCH4, ADC14, GPIO |
| 12 | TOUCH6, ADC15, GPIO |
| — | GND, 3.3V |

**Правый ряд:**
| Пин | Функции |
|---|---|
| 36 | ADC0, GPIO (input only) |
| 37 | GPIO (input only) |
| 38 | GPIO (input only) |
| 39 | ADC3, GPIO (input only) |
| 32 | ADC4, TOUCH9, GPIO |
| 33 | ADC5, TOUCH8, GPIO |
| 25 | ADC18, DAC1, GPIO |
| 26 | ADC19, DAC2, GPIO |
| 27 | ADC17, TOUCH7, GPIO |
| — | 3.3V, 5V, GND |

**Нижние пины:** GPIO10, GPIO35

---

### Питание и зарядка

| Параметр | Значение |
|---|---|
| Питание USB | 5V / 1A |
| Зарядный ток | 500 mA |
| Аккумулятор | 3.7V Li-Ion |
| Разъём батареи | JST 2Pin 1.25mm |
| USB | Type-C |

> ✅ **Встроенная схема зарядки** — в отличие от многих ESP32-модулей, плата заряжает Li-Ion аккумулятор напрямую через USB.

---

### Wi-Fi

| Параметр | Значение |
|---|---|
| Протокол | 802.11 b/g/n (до 150 Mbps) |
| A-MPDU / A-MSDU | поддерживается |
| Protection interval | 0.4 µS |
| Частота | 2.4 GHz (2400–2483.5 MHz) |
| Мощность передачи | 22 dBm |
| Дальность связи | до 300 м |
| Сертификаты | FCC, CE-RED, IC, TELEC, KCC, SRRC, NCC |

---

### Платформы разработки
- Arduino IDE
- MicroPython
