#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>

int main() {
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);

    printf("Test-init\n");
    int rezultat = fork();


    if (rezultat == 0) {
        execlp("/bin/sh", "sh", NULL);
        perror("execlp");
        exit(1);
    } 


    while (1) {
    wait(NULL);
   }

   return 0;
}




