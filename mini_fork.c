#include <stdio.h>
#include <unistd.h>

int main() {
    printf("do\n");
    int rezultat = fork();
    if (rezultat == 0) {
        printf("child\n");
    } else {
        printf("parent\n");
    }
    return 0;
}