#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    printf("do\n");
    int rezultat = fork();

    if (rezultat == 0) {
        execlp("ls", "ls", NULL);
        perror("execlp");
        exit(1);
    } else {
        wait(NULL); 
        printf("parent\n");

    }
    return 0;
}