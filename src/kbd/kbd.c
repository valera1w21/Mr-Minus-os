#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define TOUCH_DEV "/dev/input/event1"

#include "font8x8_basic.h"
#define font_bitmap font8x8_basic

struct key_def { const char *label; int code; int width_units; };

static const struct key_def rows[5][11] = {
    {{"1",KEY_1,1},{"2",KEY_2,1},{"3",KEY_3,1},{"4",KEY_4,1},{"5",KEY_5,1},
     {"6",KEY_6,1},{"7",KEY_7,1},{"8",KEY_8,1},{"9",KEY_9,1},{"0",KEY_0,1},{0}},
    {{"q",KEY_Q,1},{"w",KEY_W,1},{"e",KEY_E,1},{"r",KEY_R,1},{"t",KEY_T,1},
     {"y",KEY_Y,1},{"u",KEY_U,1},{"i",KEY_I,1},{"o",KEY_O,1},{"p",KEY_P,1},{0}},
    {{"a",KEY_A,1},{"s",KEY_S,1},{"d",KEY_D,1},{"f",KEY_F,1},{"g",KEY_G,1},
     {"h",KEY_H,1},{"j",KEY_J,1},{"k",KEY_K,1},{"l",KEY_L,1},{"-",KEY_MINUS,1},{0}},
    {{"z",KEY_Z,1},{"x",KEY_X,1},{"c",KEY_C,1},{"v",KEY_V,1},{"b",KEY_B,1},
     {"n",KEY_N,1},{"m",KEY_M,1},{".",KEY_DOT,1},{"/",KEY_SLASH,1},{"<-",KEY_BACKSPACE,1},{0}},
    {{"space",KEY_SPACE,7},{"enter",KEY_ENTER,3},{0}},
};
struct key { const char *label; int code; int x, y, w, h; };
static struct key keys[64];
static int key_count = 0;

static struct fb_var_screeninfo vi;
static struct fb_fix_screeninfo fi;
static uint8_t *screen;
static int fbfd;

static uint32_t rgb(int r, int g, int b) {
    return ((uint32_t)(r >> (8 - vi.red.length))   << vi.red.offset)
         | ((uint32_t)(g >> (8 - vi.green.length)) << vi.green.offset)
         | ((uint32_t)(b >> (8 - vi.blue.length))  << vi.blue.offset);
}

static void put_pixel(int x, int y, uint32_t c) {
    if (x < 0 || y < 0 || x >= (int)vi.xres || y >= (int)vi.yres) return;
    long o = (long)(y + vi.yoffset) * fi.line_length + (long)(x + vi.xoffset) * (vi.bits_per_pixel / 8);
    if (vi.bits_per_pixel == 32) *(uint32_t *)(screen + o) = c;
    else if (vi.bits_per_pixel == 16) *(uint16_t *)(screen + o) = (uint16_t)c;
}

static void fill_rect(int x, int y, int w, int h, uint32_t c) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            put_pixel(i, j, c);
}

static void draw_char(char ch, int x, int y, int m, uint32_t c) {
    unsigned char k = (unsigned char)ch;
    if (k >= 128) return;
    for (int str = 0; str < 8; str++)
        for (int st = 0; st < 8; st++)
            if ((unsigned char)font_bitmap[k][str] & (1 << st))
                fill_rect(x + st * m, y + str * m, m, m, c);
}

static void draw_text_centered(const char *s, int x, int y, int w, int h, uint32_t c) {
    int m = 4;
    int dl = (int)strlen(s) * 8 * m;
    while (dl > w - 8 && m > 1) { m--; dl = (int)strlen(s) * 8 * m; }
    int sx = x + (w - dl) / 2, sy = y + (h - 8 * m) / 2;
    for (int i = 0; s[i]; i++) draw_char(s[i], sx + i * 8 * m, sy, m, c);
}

static int kbd_top;

static void draw_keyboard(int pressed) {
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vi);
    fill_rect(0, kbd_top, vi.xres, vi.yres - kbd_top, rgb(20, 20, 20));
    for (int i = 0; i < key_count; i++) {
        struct key *k = &keys[i];
        uint32_t fon = (i == pressed) ? rgb(0, 120, 210) : rgb(70, 70, 70);
        fill_rect(k->x + 3, k->y + 3, k->w - 6, k->h - 6, fon);
        draw_text_centered(k->label, k->x, k->y, k->w, k->h, rgb(255, 255, 255));
    }
}

static void layout_keys(void) {
    int kbd_height = vi.yres * 2 / 5;
    kbd_top = vi.yres - kbd_height;
    int h = kbd_height / 5, unit_width = vi.xres / 10;
    for (int r = 0; r < 5; r++) {
        int x = 0;
        for (int i = 0; rows[r][i].label; i++) {
            struct key *k = &keys[key_count++];
            k->label = rows[r][i].label;
            k->code = rows[r][i].code;
            k->x = x; k->y = kbd_top + r * h;
            k->w = unit_width * rows[r][i].width_units; k->h = h;
            x += k->w;
        }
    }
    }

static int key_at(int x, int y) {
    for (int i = 0; i < key_count; i++) {
        struct key *k = &keys[i];
        if (x >= k->x && x < k->x + k->w && y >= k->y && y < k->y + k->h) return i;
    }
    return -1;
}

static int uinput_fd;

static void send_event(int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type; ev.code = code; ev.value = value;
    write(uinput_fd, &ev, sizeof(ev));
}

static void press_key(int code) {
    send_event(EV_KEY, code, 1); send_event(EV_SYN, SYN_REPORT, 0);
    send_event(EV_KEY, code, 0); send_event(EV_SYN, SYN_REPORT, 0);
}

static int create_uinput(void) {
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) { perror("/dev/uinput"); return -1; }
    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);
    for (int i = 0; i < key_count; i++) ioctl(uinput_fd, UI_SET_KEYBIT, keys[i].code);
    struct uinput_setup us;
    memset(&us, 0, sizeof(us));
    us.id.bustype = BUS_VIRTUAL;
    us.id.vendor = 0x1234; us.id.product = 0x5678;
    strcpy(us.name, "minus-klaviatura");
    if (ioctl(uinput_fd, UI_DEV_SETUP, &us) < 0 || ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        perror("uinput setup"); return -1;
    }
    return 0;
}

static void shrink_console(void) {
    int t = open("/dev/tty1", O_RDWR);
    if (t < 0) return;
    struct winsize ws;
    if (ioctl(t, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        ws.ws_row = ws.ws_row * 3 / 5;
        ioctl(t, TIOCSWINSZ, &ws);
    }

     close(t);
}

int main(void) {
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd < 0) { perror("/dev/fb0"); return 1; }
    ioctl(fbfd, FBIOGET_FSCREENINFO, &fi);
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vi);
    screen = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (screen == MAP_FAILED) { perror("mmap"); return 1; }

    layout_keys();
    if (create_uinput() < 0) return 1;
    shrink_console();

    int tfd = open(TOUCH_DEV, O_RDONLY);
    if (tfd < 0) { perror(TOUCH_DEV); return 1; }

    struct input_absinfo ax, ay;
    int ex = ioctl(tfd, EVIOCGABS(ABS_X), &ax);
    int ey = ioctl(tfd, EVIOCGABS(ABS_Y), &ay);
    if (ex < 0 || ax.maximum <= ax.minimum) { ax.minimum = 0; ax.maximum = vi.xres - 1; }
    if (ey < 0 || ay.maximum <= ay.minimum) { ay.minimum = 0; ay.maximum = vi.yres - 1; }

    draw_keyboard(-1);

     int sx = 0, sy = 0, finger_down = 0, just_touched = 0, pressed = -1;
    struct pollfd p = { .fd = tfd, .events = POLLIN };

    while (1) {
        int r = poll(&p, 1, 700);
        if (r == 0) { draw_keyboard(pressed); continue; }
        if (r < 0) continue;

        struct input_event ev;
        if (read(tfd, &ev, sizeof(ev)) != sizeof(ev)) continue;

        if (ev.type == EV_ABS && ev.code == ABS_X)
            sx = (ev.value - ax.minimum) * (int)vi.xres / (ax.maximum - ax.minimum + 1);
        else if (ev.type == EV_ABS && ev.code == ABS_Y)
            sy = (ev.value - ay.minimum) * (int)vi.yres / (ay.maximum - ay.minimum + 1);
        else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            if (ev.value == 1) { finger_down = 1; just_touched = 1; }
            else {
                finger_down = 0;
                if (pressed >= 0) press_key(keys[pressed].code);
                pressed = -1;
                draw_keyboard(-1);
            }
        }

                else if (ev.type == EV_SYN && ev.code == SYN_REPORT && finger_down) {
            int k = key_at(sx, sy);
            if (just_touched || k != pressed) {
                pressed = k;
                just_touched = 0;
                draw_keyboard(pressed);
            }
        }
    }
    return 0;
}
