// TWORZENIE PLIKU O OKREŚLONEJ W ARGUMENTACH NAZWIE, UPRAWNIENIACH I ROZMIARZE
// WYPEŁNIANIE PLIKU W OK. 10% LOSOWYMI ZNAKAMI [A-Z] I ZNAKAMI O KODZIE 0 W RESZCZIE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s -n Name -p OCTAL -s SIZE\n", pname);
    exit(EXIT_FAILURE);
}

