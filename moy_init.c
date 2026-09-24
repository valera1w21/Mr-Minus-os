#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <fcntl.h>

// загружает один модуль ядра по пути к файлу
void zagruzi(const char *put) {
    int fd = open(put, O_RDONLY);
    if (fd < 0) {
        perror(put);
        return;
    }
    if (syscall(SYS_finit_module, fd, "", 0) != 0) {
        perror(put);
    } else {
        printf("zagruzil: %s\n", put);
    }
    close(fd);
}

int main() {
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);

    for (int i = 0; i < 100; i++) {
        if (access("/dev/fb0", F_OK) == 0) break;
        usleep(100000);
    }

    int k = open("/dev/tty1", O_RDWR);
    dup2(k, 1);
    dup2(k, 2);
    printf(">>> MOY INIT ZAPUSTILSYA <<<\n");

    zagruzi("/lib/modules/hid.ko");
    zagruzi("/lib/modules/usbhid.ko");
    zagruzi("/lib/modules/hid-generic.ko");
    zagruzi("/lib/modules/evdev.ko");
    zagruzi("/lib/modules/uinput.ko");
     // экранная клавиатура — живёт в фоне всё время
    if (fork() == 0) {
        execl("/bin/moy_kbd", "moy_kbd", NULL);
        perror("moy_kbd");
        exit(1);
    }
    sleep(1);

    while (1) {
        int rezultat = fork();

        if (rezultat == 0) {
            setsid();

            int konsol = open("/dev/tty1", O_RDWR);
            dup2(konsol, 0);
            dup2(konsol, 1);
            dup2(konsol, 2);
            ioctl(0, TIOCSCTTY, 1);

            execlp("/bin/moysh", "moysh", NULL);
            perror("execlp");
            exit(1);
        }
        waitpid(rezultat, NULL, 0);
        sleep(1);
    }

    return 0;
}
