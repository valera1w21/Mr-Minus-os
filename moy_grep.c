#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{

    if (argc < 3) {
          printf("Usage: %s pattern filename\n", argv[0]);
    return 1;
    }

int errory = 0;


  FILE *f = fopen(argv[2], "r"); //I HATE ARGV

   if (f == NULL) {
    perror("fopen");
    errory = 1;
    return errory;
}

  char buffer[1024];
  while (fgets(buffer, 1024, f) != NULL){ 

  if (strstr(buffer, argv[1]) != NULL){ 
    printf("%s", buffer);
    }
}

   fclose(f);

     return errory;
}