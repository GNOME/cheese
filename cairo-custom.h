#ifndef CAIRO_CUSTOM_H
#define CAIRO_CUSTOM_H
#include <cairo.h>

void
cairo_rectangle_round (cairo_t * cr,
                       double x0,
                       double y0,
                       double width,
                       double height,
                       double radius);

#endif /* CAIRO_CUSTOM_H */
