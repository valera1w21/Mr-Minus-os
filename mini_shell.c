#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char buffer[1024];

    while (1) {
        printf("> ");
        fgets(buffer, 1024, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        int rezultat = fork();

    if (rezultat == 0) {
        execlp(buffer, buffer, NULL);
        perror("execlp");
        exit(1);
    } else {
        int status;
        wait(&status); 
       }

    }

    return 0;
}