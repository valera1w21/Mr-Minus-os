#include <stdio.h>
#include <unistd.h>

int main() {
    printf("do\n");
    fork();
    printf("posle\n");
    return 0;
}