#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "main.h"
#include "geom.h"
#include "win.h"

#include <stdio.h> /* XXX */

Window
win_create(const struct geom *geom, const char *name)
{
	Window win;
	int screen;

	screen = DefaultScreen(display);

	win = XCreateSimpleWindow(display, root,
		geom->x, geom->y, geom->w, geom->h,
		WIN_BORDER,
		BlackPixel(display, screen), WhitePixel(display, screen));

	XStoreName(display, win, name);

	win_resize(win, geom);

	XMapWindow(display, win);
	XFlush(display);

	return win;
}

void
win_resize(Window win, const struct geom *geom)
{
	assert(geom != NULL);

	XMoveResizeWindow(display, win,
		geom->x, geom->y, geom->w, geom->h);
}

void
win_border(Window win, const char *colour)
{
	Colormap cm;
	XColor col;

	assert(colour != NULL);

	cm = DefaultColormap(display, 0);

	XParseColor(display, cm, colour, &col);
	XAllocColor(display, cm, &col);

	XSetWindowBorder(display, win, col.pixel);
}

int
win_geom(Window win, struct geom *geom)
{
	int x, y;
	unsigned int w, h;
	Window rr;
	unsigned int bw;
	unsigned int depth;

	assert(geom != NULL);

	XGetGeometry(display, win, &rr, &x, &y, &w, &h, &bw, &depth);

	if (x < 0 || y < 0) {
		errno = EINVAL;
		return -1;
	}

	if (w == 0 || h == 0) {
		errno = EINVAL;
		return -1;
	}

	geom->x = x;
	geom->y = y;
	geom->w = w;
	geom->h = h;

	return 0;
}

