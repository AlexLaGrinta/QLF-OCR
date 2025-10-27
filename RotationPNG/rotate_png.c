#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>

typedef struct {
    png_byte r, g, b, a;
} rgba_t;

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

int save_png(const char *filename, rgba_t **data, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return 0;
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return 0;
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return 0;
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return 0;
    }
    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8,
                 PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_write_image(png, (png_bytepp)data);
    png_write_end(png, NULL);
    fclose(fp);
    png_destroy_write_struct(&png, &info);
    return 1;
}

rgba_t **rotate_image(rgba_t **src, int width, int height, double angle_deg, int *new_w, int *new_h) {
    double angle = angle_deg * M_PI / 180.0;
    double cos_a = cos(angle), sin_a = sin(angle);
    int w_out = (int)(fabs(width * cos_a) + fabs(height * sin_a));
    int h_out = (int)(fabs(width * sin_a) + fabs(height * cos_a));
    *new_w = w_out;
    *new_h = h_out;
    rgba_t **dst = malloc(sizeof(rgba_t *) * h_out);
    for (int y = 0; y < h_out; y++) dst[y] = calloc(w_out, sizeof(rgba_t));
    double cx = width / 2.0;
    double cy = height / 2.0;
    double ncx = w_out / 2.0;
    double ncy = h_out / 2.0;
    for (int y = 0; y < h_out; y++) {
        for (int x = 0; x < w_out; x++) {
            double xr = (x - ncx) * cos_a + (y - ncy) * sin_a + cx;
            double yr = -(x - ncx) * sin_a + (y - ncy) * cos_a + cy;
            int xi = (int)xr;
            int yi = (int)yr;
            if (xi >= 0 && xi < width && yi >= 0 && yi < height)
                dst[y][x] = src[yi][xi];
            else {
                dst[y][x].r = 255;
                dst[y][x].g = 255;
                dst[y][x].b = 255;
                dst[y][x].a = 0;
            }
        }
    }
    return dst;
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
    int new_w, new_h;
    rgba_t **rotated = rotate_image(img, width, height, angle, &new_w, &new_h);
    if (!save_png("output.png", rotated, new_w, new_h))
        fprintf(stderr, "Erreur enregistrement PNG\n");
    else
        printf("Image tournée enregistrée dans 'output.png'\n");
    free_image(img, height);
    free_image(rotated, new_h);
    return 0;
}
