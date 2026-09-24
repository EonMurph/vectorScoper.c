#include <imago2.h>
#include <math.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

#define PI 3.1415926536

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
void VS_draw_line(VS_image *img, struct point *start, double length, double angle_degrees, int thickness);
void VS_draw_circle_line(VS_image *img, struct point center, double circle_radius, double circle_angle_degrees, double length, int thickness);

void VS_put_pixel(VS_image *img, VS_pixel_RGB *pixel, struct point *coord);
void VS_set_alpha(VS_image *img, int x, int y, unsigned char alpha);

int VS_create_vectorscope(VS_image *img, int radius);
void VS_place_colour(VS_image *img, VS_pixel_RGB *colour, VS_pixel_HSV *pixel, int radius);

char outside_circle(int radius, struct point *coord);
int VS_save_png(const char *filename, VS_image *img);

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
    output_img->px = (char *)calloc(
        output_img->width * output_img->height,
        output_img->pixel_size
    );
    if (output_img->px == NULL) {
        perror("Issue allocating output pixels");
        free(output_img);
        return 1;
    }

    VS_pixel_RGB *rgb_pixel = (VS_pixel_RGB *)malloc(sizeof(VS_pixel_RGB));
    VS_pixel_HSV *hsv_pixel = (VS_pixel_HSV *)malloc(sizeof(VS_pixel_HSV));
    for (int i = 2; i < argc; i++) {
        printf("Working on %s\n", argv[i]);

        char *infile = argv[i];
        struct VS_image img;
        img.pixel_size = 4;
        void *px = img_load_pixels(infile, &img.width, &img.height, IMG_FMT_RGBA32);
        if (px == NULL) {
            fprintf(stderr, "Unable to load pixels from image: %s\n", infile);
            free(rgb_pixel);
            free(hsv_pixel);
            free(output_img->px);
            free(output_img);
            return 1;
        }
        img.px = (char *)px;

        for (int x = 0; x < img.width; x++) {
            for (int y = 0; y < img.height; y++) {
                if (VS_getpixel(&img, x, y, &rgb_pixel) != 0) {
                    return 1;
                };
                if (VS_RGB_HSV(rgb_pixel, &hsv_pixel) != 0) {
                    return 1;
                }

                VS_place_colour(output_img, rgb_pixel, hsv_pixel, radius);
            }
        }

        img_free_pixels(px);
    }
    if (VS_create_vectorscope(output_img, radius) != 0) {
        return 1;
    }

    if (VS_save_png("output.png", output_img) != 0) {
        fprintf(stderr, "Unable to save the output vectorscope image.\n");
        free(rgb_pixel);
        free(hsv_pixel);
        free(output_img->px);
        free(output_img);
        return 1;
    }

    free(rgb_pixel);
    free(hsv_pixel);
    free(output_img->px);
    free(output_img);
    return 0;
}

int VS_save_png(const char *filename, VS_image *img)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        return 1;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if (png == NULL || info == NULL) {
        png_destroy_write_struct(png != NULL ? &png : NULL, info != NULL ? &info : NULL);
        fclose(file);
        return 1;
    }

    if (setjmp(png_jmpbuf(png)) != 0) {
        png_destroy_write_struct(&png, &info);
        fclose(file);
        return 1;
    }

    png_init_io(png, file);
    png_set_IHDR(
        png,
        info,
        img->width,
        img->height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_bytep *rows = malloc(img->height * sizeof(*rows));
    if (rows == NULL) {
        png_destroy_write_struct(&png, &info);
        fclose(file);
        return 1;
    }

    for (int y = 0; y < img->height; y++) {
        rows[y] = (png_bytep)(img->px + y * img->width * img->pixel_size);
    }

    png_set_rows(png, info, rows);
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);

    free(rows);
    png_destroy_write_struct(&png, &info);
    fclose(file);
    return 0;
}

int VS_getpixel(VS_image *img, int x, int y, VS_pixel_RGB **pixel)
{
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) {
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
    return cos((PI / 180) * pixel->h);
}

double VS_get_colour_y(VS_pixel_HSV *pixel)
{
    return sin((PI / 180) * pixel->h);
}

void VS_draw_circle(VS_image *img, struct point *center, int r)
{
    VS_pixel_RGB colour = {.r = 170, .g = 138, .b = 0};
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        VS_put_pixel(img, &colour, &((struct point){center->x + x, center->y + y}));
        VS_put_pixel(img, &colour, &((struct point){center->x - x, center->y + y}));
        VS_put_pixel(img, &colour, &((struct point){center->x + x, center->y - y}));
        VS_put_pixel(img, &colour, &((struct point){center->x - x, center->y - y}));

        VS_put_pixel(img, &colour, &((struct point){center->x + y, center->y + x}));
        VS_put_pixel(img, &colour, &((struct point){center->x - y, center->y + x}));
        VS_put_pixel(img, &colour, &((struct point){center->x + y, center->y - x}));
        VS_put_pixel(img, &colour, &((struct point){center->x - y, center->y - x}));

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

    if (x < 0 || x >= img->width || y < 0 || y >= img->height) {
        return;
    }

    int loc = ((y * img->width) + x) * img->pixel_size;
    img->px[loc] = pixel->r;
    img->px[loc + 1] = pixel->g;
    img->px[loc + 2] = pixel->b;
    img->px[loc + 3] = (char)255;
}

void VS_draw_line(VS_image *img, struct point *start, double length, double angle_degrees, int thickness)
{
    VS_pixel_RGB colour = {.r = 170, .g = 138, .b = 0};

    double angle = angle_degrees * M_PI / 180.0;

    double dx = cos(angle);
    double dy = -sin(angle);

    struct point end = {
        .x = start->x + dx * length,
        .y = start->y + dy * length
    };

    int x0 = (int)lround(start->x);
    int y0 = (int)lround(start->y);
    int x1 = (int)lround(end.x);
    int y1 = (int)lround(end.y);

    int x = x0;
    int y = y0;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = abs(x1 - x0) - abs(y1 - y0);
    int radius = thickness / 2;

    for (;;) {
        for (int oy = -radius; oy <= radius; oy++) {
            for (int ox = -radius; ox <= radius; ox++) {
                if (ox * ox + oy * oy <= radius * radius) {
                    VS_put_pixel(img, &colour, &(struct point){x + ox, y + oy});
                }
            }
        }

        if (x == x1 && y == y1) {
            break;
        }

        int e2 = 2 * err;

        if (e2 > -abs(y1 - y0)) {
            err -= abs(y1 - y0);
            x += sx;
        }

        if (e2 < abs(x1 - x0)) {
            err += abs(x1 - x0);
            y += sy;
        }
    }
}
void VS_draw_circle_line(VS_image *img, struct point center, double circle_radius, double circle_angle_degrees, double length, int thickness)
{
    double theta = circle_angle_degrees * PI / 180.0;

    /*
     * 0 degrees is the left side of the circle.
     * Positive angles move from left toward the top.
     */
    struct point start = {
        .x = center.x - cos(theta) * circle_radius,
        .y = center.y - sin(theta) * circle_radius
    };

    VS_draw_line(
        img,
        &start,
        length,
        -circle_angle_degrees,
        thickness
    );
}

void VS_set_alpha(VS_image *img, int x, int y, unsigned char alpha)
{
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) {
        return;
    }

    int offset = (y * img->width + x) * img->pixel_size;
    img->px[offset + 3] = (char)alpha;
}

int VS_create_vectorscope(VS_image *img, int radius)
{
    for (int x = 0; x < img->width; x++) {
        for (int y = 0; y < img->height; y++) {
            if (!outside_circle(radius, &(struct point){x, y})) {
                VS_set_alpha(img, x, y, 255);
            }
        }
    }

    int thickness = radius / 100;
    for (int i = 0; i < thickness; i++) {
        VS_draw_circle(img, &(struct point){.x = radius, .y = radius}, radius - i);
    }
    for (double angle = 0; angle < 360; angle += 10) {
        /* double angle = 86; */
        double radians = (PI / 180) * angle;

        struct point center = {
            .x = radius,
            .y = radius,
        };

        VS_draw_circle_line(
            img,
            (struct point){radius, radius},
            radius,
            angle,
            radius / 20,
            thickness
        );
    }

    int crossbar_thickness = radius / 200;
    VS_draw_line(img, &(struct point){0, radius}, radius * 2, 0, crossbar_thickness);
    VS_draw_line(img, &(struct point){radius, 0}, radius * 2, -90, crossbar_thickness);

    return 0;
}

void VS_place_colour(VS_image *img, VS_pixel_RGB *colour, VS_pixel_HSV *pixel, int radius)
{
    int extend = pixel->s * radius / 100;
    int x = radius + lround(VS_get_colour_x(pixel) * extend);
    int y = radius - lround(VS_get_colour_y(pixel) * extend);

    int thickness = radius / 100;
    for (int i = 0; i < thickness; i++) {
        for (int j = 0; j < thickness; j++) {
            VS_put_pixel(img, colour, &(struct point){x + i, y + j});
        }
    }
}

char outside_circle(int radius, struct point *coord)
{
    int center_x = radius, center_y = radius;
    if (pow(coord->x - center_x, 2) + pow(coord->y - center_y, 2) != pow(radius, 2)) {
        int distance = sqrt(pow(coord->x - center_x, 2) + pow(coord->y - center_y, 2));
        if ((distance + 1) > radius) {
            return 1;
        }
    }

    return 0;
}
