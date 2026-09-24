#include <imago2.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

typedef struct VS_Image {
    char *px;
    int width, height;
    int pixel_size;
} VS_Image;

typedef struct VS_pixel_RGB {
    unsigned char r, g, b;
} VS_pixel_RGB;

typedef struct VS_pixel_HSV {
    int h, s, v;
} VS_pixel_HSV;

struct point {
    double x, y;
};

int VS_getpixel(VS_Image *img, int x, int y, VS_pixel_RGB **pixel);

int VS_RGB_HSV(VS_pixel_RGB *inpixel, VS_pixel_HSV **outpixel);

double VS_get_colour_x(VS_pixel_HSV *pixel);
double VS_get_colour_y(VS_pixel_HSV *pixel);

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

    struct img_pixmap output_img;
    img_init(&output_img);

    for (int i = 2; i < argc; i++) {
        printf("Working on %s\n", argv[i]);

        char *infile = argv[i];
        struct VS_Image img;
        img.pixel_size = 3;
        void *px = img_load_pixels(infile, &img.width, &img.height, IMG_FMT_RGBA32);
        if (px == NULL) {
            fprintf(stderr, "Unable to load pixels from image: %s", infile);
        }
        img.px = (char *)px;

        VS_pixel_RGB *rgb_pixel = (VS_pixel_RGB *)malloc(sizeof(VS_pixel_RGB));
        VS_pixel_HSV *hsv_pixel = (VS_pixel_HSV *)malloc(sizeof(VS_pixel_HSV));
        if (VS_getpixel(&img, 0, 0, &rgb_pixel) != 0) {
            return 1;
        };
        if (VS_RGB_HSV(rgb_pixel, &hsv_pixel) != 0) {
            return 1;
        }

        img_free_pixels(px);
    }

    return 0;
}

int VS_getpixel(VS_Image *img, int x, int y, VS_pixel_RGB **pixel)
{
    if (x >= img->width || y >= img->height) {
        fprintf(stderr, "Give a valid x, y coord within %d x %d", img->width, img->height);
        return 1;
    } else if (!pixel || *pixel == NULL) {
        fprintf(stderr, "Pixel buffer is unallocated.");
        return 1;
    }

    int offset = (y * img->width + x) * img->pixel_size;
    (*pixel)->r = *(img->px + offset);
    (*pixel)->g = *(img->px + offset + 1);
    (*pixel)->b = *(img->px + offset + 2);

    return 0;
}

int VS_RGB_HSV(VS_pixel_RGB *inpixel, VS_pixel_HSV **outpixel)
{
    if (!inpixel || !outpixel || *outpixel == NULL) {
        fprintf(stderr, "Invalid arguments given.");
        return 1;
    }

    float r, g, b, max_c, min_c, diff;
    int h, s, v;
    r = inpixel->r / 255.0;
    g = inpixel->g / 255.0;
    b = inpixel->b / 255.0;

    max_c = MAX(MAX(r, g), b);
    min_c = MIN(MIN(r, g), b);
    diff = max_c - min_c;

    // calculate hue
    if (max_c == min_c) {
        h = 0;
    } else if (max_c == r) {
        h = fmod((60 * ((g - b) / diff) + 360), 360);
    } else if (max_c == g) {
        h = fmod((60 * ((b - r) / diff) + 120), 360);
    } else if (max_c == b) {
        h = fmod((60 * ((r - g) / diff) + 240), 360);
    }

    // calculate saturation
    if (max_c == 0) {
        s = 0;
    } else {
        s = (diff / max_c) * 100;
    }

    // calculate value
    v = max_c * 100;

    (*outpixel)->h = h;
    (*outpixel)->s = s;
    (*outpixel)->v = v;

    return 0;
}

double VS_get_colour_x(VS_pixel_HSV *pixel)
{
    return cos(pixel->h);
}

double VS_get_colour_y(VS_pixel_HSV *pixel)
{
    return sin(pixel->h);
}
