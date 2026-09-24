#include <stdio.h>

int main(int argc, char *argv[])
{

    if (argc < 2) {
          printf("Usage: %s filename\n", argv[0]);
    return 1;
    }

int errory = 0;

  for(int i = 1; i < argc; ++i){

  FILE *f = fopen(argv[i], "r");

   if (f == NULL) {
    perror("fopen");
    errory = 1;
    continue;
    
}


  int ch;
   while ((ch = fgetc(f)) != EOF) {
   printf("%c", ch);
  }

   fclose(f);
  }

     return errory;
}