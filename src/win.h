#ifndef WIN_H
#define WIN_H

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

#endif

