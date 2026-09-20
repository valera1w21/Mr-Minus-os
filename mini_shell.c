#include <stdio.h>

int main() {
    char buffer[1024];

    while (1) {
        printf("> ");
        fgets(buffer, 1024, stdin);
        printf("ty vvel: %s", buffer);
    }

    return 0;
}