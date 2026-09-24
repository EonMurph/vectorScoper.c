#include <imago2.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 3) {
        perror("Please give the vectorscrope resolution radius, and the files you want colours extracted from");
        return 1;
    }
    int radius = atoi(argv[1]);
    if (radius <= 0) {
        perror("Please give the positive integer vectorscope radius as the first argument");
        return 1;
    }

    struct img_pixmap img;
    for (int i = 2; i < argc; i++) {
        printf("Working on %s\n", argv[i]);

        char *infile = argv[i];
        img_init(&img);
        if (img_load(&img, infile) == -1) {
            fprintf(stderr, "Failed to load file %s\n", infile);
        }

        printf("%d %d\n", img.width, img.height);

        img_destroy(&img);
    }

    return 0;
}
