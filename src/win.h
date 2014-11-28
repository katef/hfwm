#ifndef WIN_H
#define WIN_H

#include <X11/Xlib.h>

#define WIN_BORDER 2

struct window {
	/* TODO: placeholder */
	struct window *next;
};

Window
win_create(const struct geom *geom, const char *name, const char *class);

void
win_destroy(Window win);

int
win_resize(Window win, const struct geom *geom);

void
win_border(Window win, const char *colour);

int
win_geom(Window win, struct geom *geom);

void
win_cat(struct window **head, struct window **tail);

#endif

