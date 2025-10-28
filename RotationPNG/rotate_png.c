#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <stdint.h>

typedef struct {
    png_byte r, g, b, a;
} rgba_t;

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

rgba_t **load_png(const char *filename, int *width, int *height) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return NULL;
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return NULL;
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }
    png_init_io(png, fp);
    png_read_info(png, info);
    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);
    png_read_update_info(png, info);
    rgba_t **rows = malloc(sizeof(rgba_t *) * (*height));
    for (int y = 0; y < *height; y++) rows[y] = malloc(png_get_rowbytes(png, info));
    png_read_image(png, (png_bytepp)rows);
    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);
    return rows;
}

rgba_t **rotate_image_fixed(rgba_t **src, int width, int height, double angle_deg) {
    double angle = angle_deg * M_PI / 180.0;
    double cos_a = cos(angle), sin_a = sin(angle);

    rgba_t **dst = malloc(sizeof(rgba_t *) * height);
    for (int y = 0; y < height; y++) dst[y] = calloc(width, sizeof(rgba_t));

    double cx = width / 2.0;
    double cy = height / 2.0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double xr = (x - cx) * cos_a + (y - cy) * sin_a + cx;
            double yr = -(x - cx) * sin_a + (y - cy) * cos_a + cy;

            int x0 = (int)floor(xr);
            int y0 = (int)floor(yr);
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            double dx = xr - x0;
            double dy = yr - y0;

            rgba_t c = {255, 255, 255, 255}; // fond blanc

            if (x0 >= 0 && y0 >= 0 && x1 < width && y1 < height) {
                rgba_t p00 = src[y0][x0];
                rgba_t p10 = src[y0][x1];
                rgba_t p01 = src[y1][x0];
                rgba_t p11 = src[y1][x1];
                double w00 = (1 - dx) * (1 - dy);
                double w10 = dx * (1 - dy);
                double w01 = (1 - dx) * dy;
                double w11 = dx * dy;
                c.r = (png_byte)(p00.r * w00 + p10.r * w10 + p01.r * w01 + p11.r * w11);
                c.g = (png_byte)(p00.g * w00 + p10.g * w10 + p01.g * w01 + p11.g * w11);
                c.b = (png_byte)(p00.b * w00 + p10.b * w10 + p01.b * w01 + p11.b * w11);
            }
            dst[y][x] = c;
        }
    }
    return dst;
}

int save_bmp(const char *filename, rgba_t **data, int width, int height) {
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;
    int row_padded = (width * 3 + 3) & (~3);
    int filesize = 54 + row_padded * height;
    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};
    bfh.bfType = 0x4D42;
    bfh.bfSize = filesize;
    bfh.bfOffBits = 54;
    bih.biSize = 40;
    bih.biWidth = width;
    bih.biHeight = -height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = 0;
    bih.biSizeImage = row_padded * height;
    fwrite(&bfh, sizeof(bfh), 1, f);
    fwrite(&bih, sizeof(bih), 1, f);
    unsigned char *row = malloc(row_padded);
    for (int y = 0; y < height; y++) {
        int p = 0;
        for (int x = 0; x < width; x++) {
            row[p++] = data[y][x].b;
            row[p++] = data[y][x].g;
            row[p++] = data[y][x].r;
        }
        while (p < row_padded) row[p++] = 0;
        fwrite(row, row_padded, 1, f);
    }
    free(row);
    fclose(f);
    return 1;
}

void free_image(rgba_t **img, int height) {
    for (int i = 0; i < height; i++) free(img[i]);
    free(img);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage : %s <image.png> <angle>\n", argv[0]);
        return 1;
    }
    const char *input = argv[1];
    double angle = atof(argv[2]);
    int width, height;
    rgba_t **img = load_png(input, &width, &height);
    if (!img) {
        fprintf(stderr, "Erreur chargement PNG\n");
        return 1;
    }
    rgba_t **rotated = rotate_image_fixed(img, width, height, angle);
    if (!save_bmp("output.bmp", rotated, width, height))
        fprintf(stderr, "Erreur enregistrement BMP\n");
    else
        printf("Image tournée (dimensions conservées) enregistrée dans 'output.bmp'\n");
    free_image(img, height);
    free_image(rotated, height);
    return 0;
}

