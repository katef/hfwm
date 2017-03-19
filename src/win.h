/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef WIN_H
#define WIN_H

extern char *hostname;

extern int screen;
extern Display *display;
extern Window root;

Window
win_create(const struct geom *geom, const char *name, const char *class,
	unsigned int bw, unsigned int spacing);

void
win_destroy(Window win);

int
win_resize(Window win, const struct geom *geom,
	unsigned int bw, unsigned int spacing);

int
win_geom(Window win, struct geom *geom);

const char *
win_type(Window win);

#endif

