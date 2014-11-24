#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "main.h"
#include "geom.h"
#include "win.h"

/* XXX: debugging stuff; no error checking */
void
rectangle(int (*f)(Display *, Drawable, GC, int, int, unsigned int, unsigned int),
	struct geom *geom, const char *colour)
{
	Colormap cm;
	XColor col;
	GC gc;

	assert(f != NULL);
	assert(geom != NULL);
	assert(colour != NULL);

	cm = DefaultColormap(display, 0);

	gc = XCreateGC(display, root, 0, 0);
	XParseColor(display, cm, colour, &col);
	XAllocColor(display, cm, &col);
	XSetForeground(display, gc, col.pixel);

	f(display, root, gc, geom->x, geom->y, geom->w, geom->h);

	XFlush(display);
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

	if (!XGetGeometry(display, win, &rr, &x, &y, &w, &h, &bw, &depth)) {
		return -1;
	}

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

