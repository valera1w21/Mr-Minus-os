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

#define TACH "/dev/input/event1"

#include "font8x8_basic.h"
#define shrift font8x8_basic

struct opis { const char *nadpis; int kod; int shirina; };

static const struct opis ryady[5][11] = {
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
struct knopka { const char *nadpis; int kod; int x, y, w, h; };
static struct knopka knopki[64];
static int skolko = 0;

static struct fb_var_screeninfo vi;
static struct fb_fix_screeninfo fi;
static uint8_t *ekran;
static int fbfd;

static uint32_t cvet(int r, int g, int b) {
    return ((uint32_t)(r >> (8 - vi.red.length))   << vi.red.offset)
         | ((uint32_t)(g >> (8 - vi.green.length)) << vi.green.offset)
         | ((uint32_t)(b >> (8 - vi.blue.length))  << vi.blue.offset);
}

static void tochka(int x, int y, uint32_t c) {
    if (x < 0 || y < 0 || x >= (int)vi.xres || y >= (int)vi.yres) return;
    long o = (long)(y + vi.yoffset) * fi.line_length + (long)(x + vi.xoffset) * (vi.bits_per_pixel / 8);
    if (vi.bits_per_pixel == 32) *(uint32_t *)(ekran + o) = c;
    else if (vi.bits_per_pixel == 16) *(uint16_t *)(ekran + o) = (uint16_t)c;
}

static void pryamougolnik(int x, int y, int w, int h, uint32_t c) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            tochka(i, j, c);
}

static void bukva(char ch, int x, int y, int m, uint32_t c) {
    unsigned char k = (unsigned char)ch;
    if (k >= 128) return;
    for (int str = 0; str < 8; str++)
        for (int st = 0; st < 8; st++)
            if ((unsigned char)shrift[k][str] & (1 << st))
                pryamougolnik(x + st * m, y + str * m, m, m, c);
}

static void tekst_v_centre(const char *s, int x, int y, int w, int h, uint32_t c) {
    int m = 4;
    int dl = (int)strlen(s) * 8 * m;
    while (dl > w - 8 && m > 1) { m--; dl = (int)strlen(s) * 8 * m; }
    int sx = x + (w - dl) / 2, sy = y + (h - 8 * m) / 2;
    for (int i = 0; s[i]; i++) bukva(s[i], sx + i * 8 * m, sy, m, c);
}

static int verh_klav;

static void narisuy(int nazhata) {
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vi);
    pryamougolnik(0, verh_klav, vi.xres, vi.yres - verh_klav, cvet(20, 20, 20));
    for (int i = 0; i < skolko; i++) {
        struct knopka *k = &knopki[i];
        uint32_t fon = (i == nazhata) ? cvet(0, 120, 210) : cvet(70, 70, 70);
        pryamougolnik(k->x + 3, k->y + 3, k->w - 6, k->h - 6, fon);
        tekst_v_centre(k->nadpis, k->x, k->y, k->w, k->h, cvet(255, 255, 255));
    }
}

static void razmetka(void) {
    int visota = vi.yres * 2 / 5;
    verh_klav = vi.yres - visota;
    int h = visota / 5, dolya = vi.xres / 10;
    for (int r = 0; r < 5; r++) {
        int x = 0;
        for (int i = 0; ryady[r][i].nadpis; i++) {
            struct knopka *k = &knopki[skolko++];
            k->nadpis = ryady[r][i].nadpis;
            k->kod = ryady[r][i].kod;
            k->x = x; k->y = verh_klav + r * h;
            k->w = dolya * ryady[r][i].shirina; k->h = h;
            x += k->w;
        }
    }
    }

static int kakaya(int x, int y) {
    for (int i = 0; i < skolko; i++) {
        struct knopka *k = &knopki[i];
        if (x >= k->x && x < k->x + k->w && y >= k->y && y < k->y + k->h) return i;
    }
    return -1;
}

static int ufd;

static void poslat(int tip, int kod, int znach) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = tip; ev.code = kod; ev.value = znach;
    write(ufd, &ev, sizeof(ev));
}

static void nazhmi(int kod) {
    poslat(EV_KEY, kod, 1); poslat(EV_SYN, SYN_REPORT, 0);
    poslat(EV_KEY, kod, 0); poslat(EV_SYN, SYN_REPORT, 0);
}

static int sozday_uinput(void) {
    ufd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (ufd < 0) { perror("/dev/uinput"); return -1; }
    ioctl(ufd, UI_SET_EVBIT, EV_KEY);
    ioctl(ufd, UI_SET_EVBIT, EV_SYN);
    for (int i = 0; i < skolko; i++) ioctl(ufd, UI_SET_KEYBIT, knopki[i].kod);
    struct uinput_setup us;
    memset(&us, 0, sizeof(us));
    us.id.bustype = BUS_VIRTUAL;
    us.id.vendor = 0x1234; us.id.product = 0x5678;
    strcpy(us.name, "minus-klaviatura");
    if (ioctl(ufd, UI_DEV_SETUP, &us) < 0 || ioctl(ufd, UI_DEV_CREATE) < 0) {
        perror("uinput setup"); return -1;
    }
    return 0;
}

static void uzhmi_konsol(void) {
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
    ekran = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (ekran == MAP_FAILED) { perror("mmap"); return 1; }

    razmetka();
    if (sozday_uinput() < 0) return 1;
    uzhmi_konsol();

    int tfd = open(TACH, O_RDONLY);
    if (tfd < 0) { perror(TACH); return 1; }

    struct input_absinfo ax, ay;
    int ex = ioctl(tfd, EVIOCGABS(ABS_X), &ax);
    int ey = ioctl(tfd, EVIOCGABS(ABS_Y), &ay);
    if (ex < 0 || ax.maximum <= ax.minimum) { ax.minimum = 0; ax.maximum = vi.xres - 1; }
    if (ey < 0 || ay.maximum <= ay.minimum) { ay.minimum = 0; ay.maximum = vi.yres - 1; }

    narisuy(-1);

     int sx = 0, sy = 0, palec = 0, novoe = 0, nazhata = -1;
    struct pollfd p = { .fd = tfd, .events = POLLIN };

    while (1) {
        int r = poll(&p, 1, 700);
        if (r == 0) { narisuy(nazhata); continue; }
        if (r < 0) continue;

        struct input_event ev;
        if (read(tfd, &ev, sizeof(ev)) != sizeof(ev)) continue;

        if (ev.type == EV_ABS && ev.code == ABS_X)
            sx = (ev.value - ax.minimum) * (int)vi.xres / (ax.maximum - ax.minimum + 1);
        else if (ev.type == EV_ABS && ev.code == ABS_Y)
            sy = (ev.value - ay.minimum) * (int)vi.yres / (ay.maximum - ay.minimum + 1);
        else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            if (ev.value == 1) { palec = 1; novoe = 1; }
            else {
                palec = 0;
                if (nazhata >= 0) nazhmi(knopki[nazhata].kod);
                nazhata = -1;
                narisuy(-1);
            }
        }

                else if (ev.type == EV_SYN && ev.code == SYN_REPORT && palec) {
            int k = kakaya(sx, sy);
            if (novoe || k != nazhata) {
                nazhata = k;
                novoe = 0;
                narisuy(nazhata);
            }
        }
    }
    return 0;
}
