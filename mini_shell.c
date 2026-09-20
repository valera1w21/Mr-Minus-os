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

        char *slova[64];
        int n = 0;

        char *slovo = strtok(buffer, " ");
        while (slovo != NULL) {
            slova[n] = slovo;
            n++;
            slovo = strtok(NULL, " ");
        }
        slova[n] = NULL;

        int rezultat = fork();
        if (rezultat == 0) {
            execvp(slova[0], slova);
            perror("execvp");
            exit(1);
        } else {
            int status;
            wait(&status);
        }
    }

    return 0;
}