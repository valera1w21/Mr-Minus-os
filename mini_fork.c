#include <stdio.h>
#include <unistd.h>

int main() {
    printf("do\n");
    int rezultat = fork();
printf("posle, u menya %d\n", rezultat);
    return 0;
}