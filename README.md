# AVR RGBW Controller

RGBW контроллер на `ATtiny4313` с управлением через энкодер и отображением информации на `TM1637` дисплее.

## Возможности

- Управление RGBW светодиодной лентой
- Несколько ручных режимов
- Fade режим
- Настройка яркости и насыщенности
- Сохранение параметров в EEPROM
- Управление энкодером с кнопкой
- Отображение параметров на TM1637
- Поддержка UART режима

---

# Используемое оборудование

## Микроконтроллер
- `ATtiny4313`
- Частота: `20 MHz`

## Подключение

| Pin | Function |
|---|---|
| PD2 | Encoder A |
| PD3 | Encoder B |
| PD4 | Button |
| PD5 | RGB output |
| PD6 | Status LED |
| PB0 | TM1637 CLK |
| PB1 | TM1637 DIO |

---

# Режимы работы

## UART

Отображение ошибок UART:
- FE
- DOR
- UPE

## MANUAL1-3

Ручное управление:
- Brightness
- Red
- Green
- Blue
- White

## FADE

Автоматическая смена цветов:
- Speed
- Mode
- Brightness
- Saturation

## SAVE

Сохранение текущих параметров в EEPROM.

---

# Сборка

## Зависимости

- `avr-gcc`
- `avr-libc`
- `make`
- `avrdude`

## Компиляция

```bash
make
```

## Прошивка

```bash
avrdude -p t4313 -c usbasp -U flash:w:firmware.hex
```

---

# Fuse bits

Пример настройки fuse битов для внешнего генератора `20 MHz`:

```bash
avrdude -p t4313 -c usbasp \
-U lfuse:w:0xFF:m \
-U hfuse:w:0xDF:m \
-U efuse:w:0xFF:m
```

---

# TODO

- Улучшить обработку энкодера
- Добавить UART protocol
- Оптимизировать размер прошивки
- Добавить дополнительные эффекты

---

# License

MIT
