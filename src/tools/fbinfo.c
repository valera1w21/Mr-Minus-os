#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

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

    printf("shirina: %u\n", vi.xres);
    printf("visota: %u\n", vi.yres);
    printf("depth: %u\n", vi.bits_per_pixel);
    printf("line_length: %u\n", fi.line_length);
    printf("smem_len: %u\n", fi.smem_len);
    printf("type: %u\n", fi.type);
    printf("ywrapstep: %u\n", fi.ywrapstep);
    printf("capabilities: %u\n", fi.capabilities);
    printf("type_aux: %u\n", fi.type_aux);

    close(fbfd);
    return 0;
}