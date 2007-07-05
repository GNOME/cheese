#ifndef __CAIRO_CUSTOM_H__
#define __CAIRO_CUSTOM_H__
#include <cairo.h>

void
cairo_rectangle_round (cairo_t * cr,
                       double x0,
                       double y0,
                       double width,
                       double height,
                       double radius);

#endif /* __CAIRO_CUSTOM_H__ */
