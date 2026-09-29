# Архитектура интерфейса Minus

Здесь описано, **что делает каждый файл** и **какие функции он даёт наружу**. Код внутри пишешь сам — здесь только «что», а не «как». Имена функций — предложение, можешь менять, главное — держать один стиль.

## Раскладка

```
src/
├── ui/                        ← всё, что РИСУЕТ. Не трогает /sys напрямую
│   ├── main.c
│   ├── core/                  ← фундамент, пишется ПЕРВЫМ
│   │   ├── fb.c / fb.h
│   │   ├── draw.c / draw.h
│   │   ├── text.c / text.h
│   │   ├── image.c / image.h
│   │   ├── input.c / input.h
│   │   └── time.c / time.h
│   ├── widgets/               ← детали, из которых собираются экраны
│   │   ├── button.c / button.h
│   │   ├── window.c / window.h
│   │   └── keyboard.c / keyboard.h
│   └── screens/               ← сами экраны
│       ├── boot_splash.c / .h
│       ├── lock_screen.c / .h
│       ├── desktop.c / .h
│       ├── taskbar.c / .h
│       ├── start_menu.c / .h
│       └── quick_settings.c / .h
├── system/                    ← всё, что УПРАВЛЯЕТ ЖЕЛЕЗОМ. Ничего не рисует
│   ├── battery.c, brightness.c, clock.c, wifi.c, bluetooth.c,
│   ├── flashlight.c, airplane.c, rotation.c, power_saver.c, power.c
│   └── (у каждого свой .h)
└── auth/                      ← проверка, что это ты
    ├── pin.c / pin.h
    └── account.c / account.h
```

**Два главных правила:**
1. Файлы из `ui/` никогда не читают и не пишут `/sys` сами. Нужен процент батареи — зовут `battery_percent()` из `system/battery.h`.
2. Файлы из `system/` никогда не рисуют. Они только отвечают на вопросы и выполняют команды.

Так дизайн можно поменять, не трогая железо, а телефон — не трогая дизайн.

У каждого `.h` — **защита от двойного подключения** (ищи `include guard`):
```
#ifndef FB_H
#define FB_H
   ... объявления функций ...
#endif
```

---

## ui/core — фундамент

### fb.c — экран
Единственный файл, который знает про `/dev/fb0`.
- `int fb_open(void)` — открыть `/dev/fb0`, узнать размеры (`FBIOGET_VSCREENINFO`, `FBIOGET_FSCREENINFO`), сделать `mmap`, выделить **свой буфер** (`malloc` на весь экран), забрать экран у консоли: открыть `/dev/tty1` и `ioctl(tty, KDSETMODE, KD_GRAPHICS)`. Вернуть 0 или -1.
- `void fb_close(void)` — вернуть `KD_TEXT`, освободить память. **Обязательно вызывается при любом выходе.**
- `uint32_t *fb_buffer(void)` — указатель на твой буфер. Все рисуют сюда, а не на экран.
- `int fb_width(void)`, `int fb_height(void)`
- `void fb_present(void)` — скопировать весь буфер на экран (`memcpy` построчно, учитывая `line_length`).
- `void fb_present_rect(int x, int y, int w, int h)` — скопировать только кусок. Для часов на панели — чтобы не перерисовывать весь экран.
- `uint32_t fb_rgb(int r, int g, int b)` — собрать цвет в формате экрана (через `red.offset` и т. д.).

Ищи: `double buffering framebuffer`, `KDSETMODE KD_GRAPHICS`.

### draw.c — рисование фигур
Работает только с буфером из `fb`.
- `fill_rect(x, y, w, h, color)` — залитый прямоугольник. Заливай **строками**, а не точками — быстрее.
- `draw_rect(x, y, w, h, color)` — только рамка.
- `fill_round_rect(x, y, w, h, radius, color)` — со скруглёнными углами (кнопки XP).
- `fill_gradient_v(x, y, w, h, top, bottom)` — вертикальный перелив. **Главная функция для стиля XP**: панель задач, шапки окон, кнопки.
- `draw_line(x1, y1, x2, y2, color)` — ищи `Bresenham line algorithm`.
- `fill_circle(cx, cy, r, color)` — ищи `midpoint circle algorithm`.
- `blend(color_under, color_over, alpha)` — смешать цвета, для полупрозрачного. Ищи `alpha blending formula`.
- Все функции должны **обрезать** то, что выходит за край экрана, — иначе запишешь в чужую память и упадёшь.

### text.c — буквы
- `int text_init(void)` — загрузить шрифт `NotoSans-Regular.ttf` из `/usr/share/minus/fonts/`.
- `void text_draw(x, y, const char *s, int size, uint32_t color)` — нарисовать строку.
- `int text_width(const char *s, int size)` — ширина строки в пикселях. Нужна, чтобы ставить текст по центру кнопки.
- Для начала можно оставить шрифт 8×8 из клавиатуры, потом перейти на **stb_truetype** (github.com/nothings/stb) — он умеет `.ttf`. Русские буквы приходят в UTF-8 по 2 байта — ищи `UTF-8 decode`.

### image.c — картинки
- `struct image { int w, h; uint32_t *pixels; }`
- `struct image *image_load(const char *path)` — загрузить PNG через **stb_image**.
- `void image_draw(const struct image *img, int x, int y)` — нарисовать с учётом прозрачности (альфа-канал).
- `void image_free(struct image *img)`

### input.c — касания
- `int input_open(void)` — найти тачскрин (пока можно `/dev/input/event1`, потом — перебором и `EVIOCGBIT`), узнать диапазон координат (`EVIOCGABS`).
- `int input_fd(void)` — номер файла, чтобы `main.c` мог ждать его в `poll`.
- `int input_read(struct touch *t)` — прочитать события и собрать их в одно понятное:
  ```
  struct touch { int type; int x; int y; };   // type: TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP
  ```
  Координаты сразу переведены в пиксели экрана. Всё знание про `EV_ABS`, `BTN_TOUCH`, `SYN_REPORT` живёт только здесь.

### time.c — время для анимаций и таймеров
- `long now_ms(void)` — миллисекунды через `clock_gettime(CLOCK_MONOTONIC)`. Не «сколько времени на часах», а «сколько прошло» — для таймаута экрана и анимаций.

---

## ui/widgets — детали

### button.c — кнопка
- `struct button { int x, y, w, h; const char *label; struct image *icon; int pressed; void (*on_click)(void); }`
- `button_draw(const struct button *b)` — нарисовать в стиле XP (перелив + рамка), нажатая — темнее.
- `int button_hit(const struct button *b, int x, int y)` — попал ли палец в кнопку.
- `on_click` — **указатель на функцию**: что сделать при нажатии. Ищи `function pointer callback C`.

### window.c — окно в стиле XP
- `struct window { int x, y, w, h; const char *title; struct image *icon; }`
- `window_draw(const struct window *w)` — синяя шапка с переливом, иконка, заголовок, красный крестик, светлое тело.
- `int window_close_hit(const struct window *w, int x, int y)` — нажали ли крестик.
- `void window_body(const struct window *w, int *x, int *y, int *bw, int *bh)` — где внутри окна можно рисовать содержимое.

### keyboard.c — клавиатура
Твой `kbd.c`, но **внутри интерфейса**: не отдельная программа и без `uinput`.
- `keyboard_show()`, `keyboard_hide()`, `int keyboard_visible(void)`
- `keyboard_draw()` — в стиле XP: светлые клавиши, синие служебные, зелёный Enter. С Shift, Ctrl и `_`.
- `int keyboard_touch(const struct touch *t, char *out)` — обработать касание, вернуть 1 и букву в `out`, если нажата клавиша. Буква идёт тому, кто сейчас ждёт ввод (поле PIN, терминал).

---

## ui/screens — экраны

У **каждого экрана одинаковый набор функций** — так `main.c` может обращаться с любым одинаково:
- `X_init()` — подготовить (загрузить картинки).
- `X_draw()` — нарисовать себя в буфер.
- `X_touch(const struct touch *t)` — обработать касание.
- `int X_tick(long now)` — «прошло время»; вернуть 1, если надо перерисоваться (сменилась минута).

### boot_splash.c
Чёрный фон, логотип `minus-logo`, надпись, бегущие зелёные квадратики. Показывается, пока всё загружается, потом переключает на `lock_screen`.

### lock_screen.c
Обои, крупные часы и дата (`clock.h`), карточка пользователя (`account.h`), поле PIN. Ввод — через `keyboard` или свою цифровую клавиатуру. Проверка — `pin_check()` из `auth/pin.h`. Верно → `desktop`. Неверно → потрясти поле и ждать секунду.

### desktop.c
Обои (`image_draw`) и значки приложений на рабочем столе (массив `button`). Обои рисуются **один раз** в отдельную картинку и потом просто копируются.

### taskbar.c
Нижняя панель: зелёная кнопка **minus** (открывает `start_menu`), кнопки открытых окон, трей справа — Wi-Fi, батарея, часы. Данные берёт из `system/`. Перерисовывается **раз в минуту** или когда что-то поменялось, и только своей полоской (`fb_present_rect`).

### start_menu.c
Меню над кнопкой **minus**: шапка с именем, список приложений, «All apps», внизу Restart и Power off (`power.h`).

### quick_settings.c
Окошко из трея: шесть переключателей, ползунок яркости, блок «о системе», Console и Power off. Каждая кнопка зовёт функцию из `system/` — например `flashlight_set(1)`.

---

## ui/main.c — дирижёр

Порядок:
1. `fb_open()`, `input_open()`, `text_init()`, загрузить картинки.
2. Повесить `fb_close()` на выход и на падения — `atexit` и `sigaction` для `SIGTERM`, `SIGSEGV`. Иначе при падении экран останется чёрным.
3. Показать `boot_splash`.
4. **Главный цикл:**
   - `poll()` ждёт касание **или** истечение времени до ближайшего события (смена минуты, конец анимации, таймаут экрана). Программа **спит**, а не крутится вхолостую.
   - Пришло касание → отдать тому, кто сверху: клавиатура → открытое меню/шторка → текущий экран.
   - Прошло время → вызвать `X_tick` у экрана и панели.
   - Если кто-то сказал «надо перерисовать» → нарисовать в буфер → `fb_present` (или только изменившийся кусок).
5. Экраны переключаются как **состояния**: `SPLASH → LOCK → DESKTOP`, поверх могут быть `START_MENU`, `QUICK_SETTINGS`, `KEYBOARD`. Ищи: `state machine C`.
6. Нет касаний N секунд → `brightness_screen_off()` и спать, пока не коснутся.

Проверка правильности: запускаешь, ничего не трогаешь, открываешь `top` — у программы должно быть **0%** процессора.

---

## system/ — железо

Каждый файл отвечает на вопрос или выполняет команду. Пути в `/sys` на S3 проверь на телефоне через `ls` — у разных телефонов они разные, поэтому держи их в одном месте, в начале файла.

| Файл | Функции | Откуда берёт |
|---|---|---|
| `battery.c` | `battery_percent()`, `battery_charging()` | `/sys/class/power_supply/*/capacity` и `status`; тип зарядки — `/sys/class/extcon/extcon0/state` (`USB`, `SDP`…) |
| `brightness.c` | `brightness_get()`, `brightness_set(0..100)`, `brightness_screen_off()`, `brightness_screen_on()` | `/sys/class/backlight/*/brightness`; выключить экран — `FBIOBLANK` |
| `clock.c` | `clock_now(struct tm *)`, `clock_format(buf, "12:45")`, `clock_minute_changed()` | `time()`, `localtime()`, `strftime()`; часовой пояс — переменная `TZ` |
| `wifi.c` | `wifi_enabled()`, `wifi_set(on)` | пока заглушка «выключено»; потом `rfkill` и `/sys/class/net/wlan0/` |
| `bluetooth.c` | `bluetooth_enabled()`, `bluetooth_set(on)` | пока заглушка; потом `rfkill` |
| `flashlight.c` | `flashlight_get()`, `flashlight_set(on)` | `/sys/class/leds/*/brightness` — найди, как называется вспышка |
| `airplane.c` | `airplane_get()`, `airplane_set(on)` | сам ничего не трогает — выключает Wi-Fi и Bluetooth через их функции |
| `rotation.c` | `rotation_locked()`, `rotation_set_locked(on)`, `rotation_current()` | акселерометр: `/sys/bus/iio/devices/iio:device*/in_accel_*_raw` |
| `power_saver.c` | `power_saver_get()`, `power_saver_set(on)` | ниже яркость, короче таймаут, режим процессора в `/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor` |
| `power.c` | `power_reboot()`, `power_off()` | `sync()` и системный вызов `reboot()` |

---

## auth/ — проверка

### pin.c
- `int pin_is_set(void)`
- `int pin_set(const char *pin)` — сохранить **не сам PIN, а его отпечаток** (хэш). Ищи `password hashing`, `salt`.
- `int pin_check(const char *pin)` — посчитать отпечаток и сравнить. После нескольких ошибок подряд — ждать всё дольше.
- Где хранить: `/etc/minus/pin`. **Внимание**: сейчас система живёт в ramdisk, и после перезагрузки файл пропадёт. Пока можно так, но для настоящей блокировки нужен раздел на карте или внутренней памяти.

### account.c
- `const char *account_name(void)`, `const char *account_avatar(void)` — имя и картинка пользователя из `/etc/minus/account.conf`.

`password.c` пока не нужен — PIN хватит. Появится длинный пароль — добавишь.

---

## Где что лежит на телефоне

- программа интерфейса → `/bin/minus-ui`
- картинки и шрифты → `/usr/share/minus/` (копия папки `assets/`)
- настройки → `/etc/minus/`

И в `init.c`: запускать `/bin/minus-ui` вместо `msh`. Если он упал — вернуть текстовую консоль и запустить `mkbd` и `msh`, как сейчас. Это страховка.

---

## В каком порядке писать

Каждый шаг заканчивается тем, что видно на экране:

1. **`fb.c` + `draw.c` + `main.c`** — забрать экран, залить фон, нарисовать прямоугольник, уснуть. В `top` — 0%.
2. **`fill_gradient_v`** — нарисовать панель задач в стиле XP.
3. **`text.c`** — на панели появились часы.
4. **`image.c`** — появились обои и иконки.
5. **`input.c` + `button.c`** — кнопка **minus** подсвечивается при нажатии.
6. **`start_menu.c`** — меню открывается и закрывается.
7. **`window.c`** — открывается окно с крестиком.
8. **`keyboard.c`** — клавиатура внутри интерфейса.
9. **`lock_screen.c` + `pin.c`** — блокировка с PIN.
10. **`quick_settings.c` + `system/`** — фонарик, яркость, батарея.
