#ifndef ROTATE_PNG_H
#define ROTATE_PNG_H

#include <gdk-pixbuf/gdk-pixbuf.h>

GdkPixbuf* rotate_pixbuf_bilinear_fixed(const GdkPixbuf *src, double angle_deg);
double detect_skew_angle(const GdkPixbuf *src);
int automatic_rotate(const char* input_path, const char* output_path);

#endif

