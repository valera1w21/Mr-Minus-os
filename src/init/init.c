#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <fcntl.h>

// загружает один модуль ядра по пути к файлу
void load_module(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        return;
    }
    if (syscall(SYS_finit_module, fd, "", 0) != 0) {
        perror(path);
    } else {
        printf("loaded: %s\n", path);
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
    printf(">>> MINUS INIT STARTED <<<\n");

    load_module("/lib/modules/hid.ko");
    load_module("/lib/modules/usbhid.ko");
    load_module("/lib/modules/hid-generic.ko");
    load_module("/lib/modules/evdev.ko");
    load_module("/lib/modules/uinput.ko");
     // экранная клавиатура — живёт в фоне всё время
    if (fork() == 0) {
        execl("/bin/mkbd", "mkbd", NULL);
        perror("mkbd");
        exit(1);
    }
    sleep(1);

    while (1) {
        int pid = fork();

        if (pid == 0) {
            setsid();

            int console_fd = open("/dev/tty1", O_RDWR);
            dup2(console_fd, 0);
            dup2(console_fd, 1);
            dup2(console_fd, 2);
            ioctl(0, TIOCSCTTY, 1);

            execlp("/bin/msh", "msh", NULL);
            perror("execlp");
            exit(1);
        }
        waitpid(pid, NULL, 0);
        sleep(1);
    }

    return 0;
}
