#ifndef WIN_H
#define WIN_H

#include <X11/Xlib.h>

struct window {
	Window win;
	struct window *next;
};

Window
win_create(const struct geom *geom, const char *name, const char *class,
	unsigned int bw, unsigned int spacing);

void
win_destroy(Window win);

int
win_resize(Window win, const struct geom *geom,
	unsigned int bw, unsigned int spacing);

void
win_border(Window win, const char *colour);

int
win_geom(Window win, struct geom *geom);

void
win_cat(struct window **head, struct window **tail);

struct window *
win_find(const struct window *head, Window win);

struct window *
win_add(struct window **head, Window win);

void
win_remove(struct window **head, Window win);

#endif

