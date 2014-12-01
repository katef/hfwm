#include <assert.h>
#include <stdio.h> /* XXX */

#include <X11/Xlib.h>

#include "main.h"
#include "geom.h"
#include "order.h"
#include "layout.h"
#include "frame.h"
#include "client.h"
#include "tile.h"

/* XXX: debugging stuff; no error checking */
static void
rectangle(int (*f)(Display *, Drawable, GC, int, int, unsigned int, unsigned int),
	Window win, struct geom *geom, const char *colour)
{
	Colormap cm;
	XColor col;
	GC gc;

	assert(f != NULL);
	assert(geom != NULL);
	assert(colour != NULL);

	cm = DefaultColormap(display, 0);

	gc = XCreateGC(display, win, 0, 0);
	XParseColor(display, cm, colour, &col);
	XAllocColor(display, cm, &col);
	XSetForeground(display, gc, col.pixel);

	f(display, win, gc, geom->x, geom->y, geom->w, geom->h);

	XFlush(display);
}

int
tile_resize(const struct frame *p)
{
	struct geom area;

	assert(p != NULL);

	area.x = 0;
	area.y = 0;
	area.h = p->geom.h;
	area.w = p->geom.w;

	if (-1 == geom_inner(&area, &area, TILE_MARGIN, TILE_SPACING + FRAME_SPACING + FRAME_BORDER)) {
		return -1;
	}

	/* height reserved for tabs */
#if 0
	area.y += 30;
	area.h -= 30;
#endif

	rectangle(XFillRectangle, p->win, &area, "#22FF22");

	/*
	 * This is worth some explanation.
	 *
	 * The approach here is to progressively divide the area unevenly
	 * into n parts, and to place a client at the first 1/nth of that space.
	 * The remaining area is then divided into n - 1 parts, and so on.
	 *
	 * This results in n even-sized windows across the area, but accounts
	 * for fractional skew should n not divide exactly.
	 */
	{
		struct geom new, old;
		unsigned n;

		old = area;

		for (n = client_count(p->u.clients); n > 0; n--) {
			struct geom gin;

			/* XXX: would rather ORDER_NEXT here. order should be an option passed to this function */
			if (n >= 2) {
				layout_split(p->layout, ORDER_PREV, &new, &old, n);
			}

			if (-1 == geom_inner(&gin, &old, TILE_BORDER, TILE_SPACING)) {
				return -1;
			}

			/* TODO: win_resize() client window */
			rectangle(XFillRectangle, p->win, &gin, "#444444");

			old = new;
		}
	}

	return 0;
}

