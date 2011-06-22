#include <cstdio>
#include <cstdlib>
#include <cstring>

int getline(char **lineptr, size_t *size, FILE *fp) {
    if (!*lineptr) *lineptr = (char*)malloc(*size + 1);
    if (!*lineptr) return -1;

    if (fgets(*lineptr, *size, fp) != NULL)
      return strlen(*lineptr);
    else
      return -1;
}
