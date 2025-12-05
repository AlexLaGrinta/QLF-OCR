#ifndef ROTATE_PNG_H
#define ROTATE_PNG_H

#include <gdk-pixbuf/gdk-pixbuf.h>

GdkPixbuf* rotate_pixbuf_bilinear_fixed(const GdkPixbuf *src, double angle_deg);
double detect_skew_angle(const GdkPixbuf *src);

#endif

