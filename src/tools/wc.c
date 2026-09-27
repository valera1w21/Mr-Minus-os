#include <stdio.h>
#include <ctype.h>

int main(int argc, char *argv[])
{

    if (argc < 2) {
          printf("Usage: %s filename\n", argv[0]);
    return 1;
    }

int errory = 0;

  for(int i = 1; i < argc; ++i){

    int symbols = 0;
    int lines = 0;
    int words = 0;
    int v_lesu = 0;

  FILE *f = fopen(argv[i], "r");

   if (f == NULL) {
    perror("fopen");
    errory = 1;
    continue;
    
}



  int ch;
   while ((ch = fgetc(f)) != EOF) {
   symbols++;

     if (isspace(ch)) {
    v_lesu = 0;
}

  if (!isspace(ch) && v_lesu == 0) {
    words++;
    v_lesu = 1;
}


   if (ch == '\n') {
    lines++;
}

  }


   printf("%d %d %d %s\n", lines, words, symbols, argv[i]);
   fclose(f);
  }

     return errory;
}