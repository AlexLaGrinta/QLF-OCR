#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rotate_png.h"

GdkPixbuf* rotate_pixbuf_bilinear_fixed(const GdkPixbuf *src, double angle_deg) {
    int w = gdk_pixbuf_get_width(src), h = gdk_pixbuf_get_height(src);
    int rowstride = gdk_pixbuf_get_rowstride(src), n = gdk_pixbuf_get_n_channels(src);
    const guchar *sp = gdk_pixbuf_get_pixels(src);
    GdkPixbuf *dst = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    guchar *dp = gdk_pixbuf_get_pixels(dst);
    int drow = gdk_pixbuf_get_rowstride(dst);
    double a = angle_deg * M_PI / 180.0, ca = cos(a), sa = sin(a);
    double cx = (w - 1) / 2.0, cy = (h - 1) / 2.0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            double rx = (x - cx) * ca + (y - cy) * sa + cx;
            double ry = -(x - cx) * sa + (y - cy) * ca + cy;
            int xi = floor(rx), yi = floor(ry);
            double dx = rx - xi, dy = ry - yi;
            guchar *d = dp + y * drow + x * 4;

            if (xi >= 0 && xi + 1 < w && yi >= 0 && yi + 1 < h) {
                const guchar *p1 = sp + yi * rowstride + xi * n;
                const guchar *p2 = sp + yi * rowstride + (xi + 1) * n;
                const guchar *p3 = sp + (yi + 1) * rowstride + xi * n;
                const guchar *p4 = sp + (yi + 1) * rowstride + (xi + 1) * n;

                for (int c = 0; c < 3; c++) {
                    double v = (1 - dx) * (1 - dy) * p1[c] +
                               dx * (1 - dy) * p2[c] +
                               (1 - dx) * dy * p3[c] +
                               dx * dy * p4[c];
                    d[c] = (guchar)v;
                }
                d[3] = 255;
            } else {
                d[0] = d[1] = d[2] = 255;
                d[3] = 255;
            }
        }
    }
    return dst;
}

double detect_skew_angle(const GdkPixbuf *src) {
    int w = gdk_pixbuf_get_width(src);
    int h = gdk_pixbuf_get_height(src);
    int row = gdk_pixbuf_get_rowstride(src);
    int n = gdk_pixbuf_get_n_channels(src);
    const guchar *p = gdk_pixbuf_get_pixels(src);

    double best_angle = 0.0;
    double best_score = -1.0;

    for (double a = -5.0; a <= 5.0; a += 0.5) {
        double proj[h];
        memset(proj, 0, sizeof(proj));

        double rad = a * M_PI / 180.0;
        double ca = cos(rad), sa = sin(rad);
        double cx = (w - 1) / 2.0, cy = (h - 1) / 2.0;

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                double rx = (x - cx) * ca + (y - cy) * sa + cx;
                double ry = -(x - cx) * sa + (y - cy) * ca + cy;
                int xi = (int)rx;
                int yi = (int)ry;
                if (xi < 0 || xi >= w || yi < 0 || yi >= h) continue;
                const guchar *sp = p + yi * row + xi * n;
                int lum = (sp[0] + sp[1] + sp[2]) / 3;
                if (lum < 128) proj[y]++;
            }
        }

        double mean = 0, var = 0;
        for (int y = 0; y < h; y++) mean += proj[y];
        mean /= h;
        for (int y = 0; y < h; y++) {
            double d = proj[y] - mean;
            var += d * d;
        }

        if (var > best_score) {
            best_score = var;
            best_angle = a;
        }
    }

    return best_angle;
}


int automatic_rotate(const char* input_path, const char* output_path)
{
    GError *err = NULL;
    GdkPixbuf *src = gdk_pixbuf_new_from_file(input_path, &err);
    if (!src) {
        fprintf(stderr, "%s\n", err->message);
        g_error_free(err);
        return 1;
    }

    double angle;
    angle = detect_skew_angle(src);

    GdkPixbuf *rotated = rotate_pixbuf_bilinear_fixed(src, angle);
    if (!gdk_pixbuf_save(rotated, output_path, "bmp", &err, NULL)) {
        fprintf(stderr, "%s\n", err->message);
        g_error_free(err);
        g_object_unref(src);
        g_object_unref(rotated);
        return 1;
    }

    g_object_unref(src);
    g_object_unref(rotated);
    return 0;
}



/*
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input.png angle|auto output.bmp\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *angle_arg = argv[2];
    const char *output_path = argv[3];
    GError *err = NULL;

    GdkPixbuf *src = gdk_pixbuf_new_from_file(input_path, &err);
    if (!src) {
        fprintf(stderr, "%s\n", err->message);
        g_error_free(err);
        return 1;
    }

    double angle;
    if (strcmp(angle_arg, "auto") == 0) angle = detect_skew_angle(src);
    else angle = atof(angle_arg);

    GdkPixbuf *rotated = rotate_pixbuf_bilinear_fixed(src, angle);
    if (!gdk_pixbuf_save(rotated, output_path, "bmp", &err, NULL)) {
        fprintf(stderr, "%s\n", err->message);
        g_error_free(err);
        g_object_unref(src);
        g_object_unref(rotated);
        return 1;
    }

    g_object_unref(src);
    g_object_unref(rotated);
    return 0;
}
*/

