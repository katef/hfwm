#ifndef WIN_H
#define WIN_H

#include <X11/Xlib.h>

struct window {
	/* TODO: placeholder */
	struct window *next;
};

/* XXX */
void
rectangle(int (*f)(Display *, Drawable, GC, int, int, unsigned int, unsigned int),
	struct geom *geom, const char *colour);

int
win_geom(Window win, struct geom *geom);

#endif

