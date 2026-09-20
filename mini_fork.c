#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    printf("do\n");
    int rezultat = fork();

    if (rezultat == 0) {
        execlp("ls", "ls", NULL);
        perror("execlp");
        exit(1);
    } else {
        printf("parent\n");

    }
    return 0;
}