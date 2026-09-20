#include <stdio.h>
#include <string.h>

int main() {
    char buffer[1024];

    while (1) {
        printf("> ");
        fgets(buffer, 1024, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        printf("ty vvel: %s", buffer);
    }

    return 0;
}