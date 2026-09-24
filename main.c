#include <imago2.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

typedef struct VS_image {
    char *px;
    int width, height;
    int pixel_size;
} VS_image;

typedef struct VS_pixel_RGB {
    unsigned char r, g, b;
} VS_pixel_RGB;

typedef struct VS_pixel_HSV {
    int h, s, v;
} VS_pixel_HSV;

struct point {
    double x, y;
};

int VS_getpixel(VS_image *img, int x, int y, VS_pixel_RGB **pixel);

int VS_RGB_HSV(VS_pixel_RGB *inpixel, VS_pixel_HSV **outpixel);

double VS_get_colour_x(VS_pixel_HSV *pixel);
double VS_get_colour_y(VS_pixel_HSV *pixel);

void VS_draw_circle(VS_image *img, struct point *center, int r);

void VS_put_pixel(VS_image *img, VS_pixel_RGB *pixel, struct point *coord);

int VS_create_vectorscope(VS_image *img, int radius);

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

    VS_image *output_img = (VS_image *)malloc(sizeof(VS_image));
    if (output_img == NULL) {
        perror("Issue allocating for output image:");
        return 1;
    }
    output_img->width = radius * 2 + 1;
    output_img->height = radius * 2 + 1;
    output_img->pixel_size = 4;
    output_img->px = (char *)calloc(output_img->width * output_img->height, sizeof(char *));

    /* for (int i = 2; i < argc; i++) { */
    /*     printf("Working on %s\n", argv[i]); */

    /*     char *infile = argv[i]; */
    /*     struct VS_image img; */
    /*     img.pixel_size = 4; */
    /*     void *px = img_load_pixels(infile, &img.width, &img.height, IMG_FMT_RGBA32); */
    /*     if (px == NULL) { */
    /*         fprintf(stderr, "Unable to load pixels from image: %s", infile); */
    /*     } */
    /*     img.px = (char *)px; */

    /*     VS_pixel_RGB *rgb_pixel = (VS_pixel_RGB *)malloc(sizeof(VS_pixel_RGB)); */
    /*     VS_pixel_HSV *hsv_pixel = (VS_pixel_HSV *)malloc(sizeof(VS_pixel_HSV)); */
    /*     if (VS_getpixel(&img, 0, 0, &rgb_pixel) != 0) { */
    /*         return 1; */
    /*     }; */
    /*     if (VS_RGB_HSV(rgb_pixel, &hsv_pixel) != 0) { */
    /*         return 1; */
    /*     } */

    /*     img_free_pixels(px); */
    /* } */
    if (VS_create_vectorscope(output_img, radius) != 0) {
        return 1;
    }

    free(output_img->px);
    free(output_img);
    return 0;
}

int VS_getpixel(VS_image *img, int x, int y, VS_pixel_RGB **pixel)
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

void VS_draw_circle(VS_image *img, struct point *center, int r)
{
    VS_pixel_RGB color = {.r = 140, .g = 108, .b = 0};
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        VS_put_pixel(img, &color, &((struct point){center->x + x, center->y + y}));
        VS_put_pixel(img, &color, &((struct point){center->x - x, center->y + y}));
        VS_put_pixel(img, &color, &((struct point){center->x + x, center->y - y}));
        VS_put_pixel(img, &color, &((struct point){center->x - x, center->y - y}));

        VS_put_pixel(img, &color, &((struct point){center->x + y, center->y + x}));
        VS_put_pixel(img, &color, &((struct point){center->x - y, center->y + x}));
        VS_put_pixel(img, &color, &((struct point){center->x + y, center->y - x}));
        VS_put_pixel(img, &color, &((struct point){center->x - y, center->y - x}));

        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }

        x++;
    }
}

void VS_put_pixel(VS_image *img, VS_pixel_RGB *pixel, struct point *coord)
{
    int x = (int)coord->x, y = (int)coord->y;
    int loc = ((y * img->width) + x) * img->pixel_size;
    img->px[loc] = pixel->r;
    img->px[loc + 1] = pixel->g;
    img->px[loc + 2] = pixel->b;
    img->px[loc + 3] = (char)255;
}

int VS_create_vectorscope(VS_image *img, int radius)
{

    int thickness = 4;
    for (int i = 0; i < thickness; i++) {
        VS_draw_circle(img, &(struct point){.x = radius, .y = radius}, radius - i);
    }
    img_save_pixels("output.jpg", (void *)img->px, img->width, img->height, IMG_FMT_RGBA32);

    return 0;
}
