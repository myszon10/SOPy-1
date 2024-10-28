#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(char *pname) {
    fprintf(stderr, "USAGE:%s ([-t x] -n Name) ... \n", pname);
    exit(EXIT_FAILURE);
}

// Mój kod
// int main(int argc, char **argv) {
//     int x = 1;

//     for(int i = 1; i < argc; i++) {
//         if(strcmp(argv[i], "-t") == 0) {
//             if(i + 1 < argc) {
//                 x = atoi(argv[i+1]);
//             }
//             else {
//                 usage(argv[0]);
//             }
//             i++;
//         }
//         else {
//             if(i + 1 < argc) {
//                 for(int j = 0; j < x; j++) {
//                     printf("Hello %s\n", argv[i+1]);
//                 }
//             }
//             else {
//                 usage(argv[0]);
//             }
//             i++;
//         }
//     }
//     return EXIT_SUCCESS;
// }

// Wzorcówka
int main(int argc, char **argv)
{
    int c, i;
    int x = 1;
    while ((c = getopt(argc, argv, "t:n:")) != -1)
        switch (c)
        {
            case 't':
                x = atoi(optarg);
                break;
            case 'n':
                for (i = 0; i < x; i++)
                    printf("Hello %s\n", optarg);
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    if (argc > optind)
        usage(argv[0]);
    return EXIT_SUCCESS;
}