#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/ioctl.h>

int main() {
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);

    printf("Test-init\n");


    while (1) {
    int rezultat = fork();


    if (rezultat == 0) {
        setsid();
        ioctl(0, TIOCSCTTY, 1);
        execlp("/bin/moysh", "moysh", NULL);
        perror("execlp");
        exit(1);
    } 
    wait(NULL);
   }

   return 0;
}




