#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdint.h>

int main() {
    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd < 0) {
        perror("/dev/fb0");
        return 1;
    }

    struct fb_var_screeninfo vi;
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vi);

    struct fb_fix_screeninfo fi;
    ioctl(fbfd, FBIOGET_FSCREENINFO, &fi);
    


    unsigned char *ekran = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (ekran == MAP_FAILED) {
    perror("mmap");
    return 1;
    }

    for (int y = 0; y < vi.yres; y++) {
       for (int x = 0; x < vi.xres; x++) {
       uint32_t *pixel = (uint32_t *)(ekran + y * fi.line_length + x * 4);
       *pixel = ((uint32_t)(x * 255 / vi.xres) << 16) | ((uint32_t)(y * 255 / vi.yres) << 8) | (uint32_t)(255 - (x * 255 / vi.xres));
       }
}

    close(fbfd);
    return 0;
}