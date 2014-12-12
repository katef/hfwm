#include <assert.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "geom.h"
#include "order.h"
#include "layout.h"
#include "frame.h"
#include "client.h"
#include "tile.h"
#include "win.h"

void
tile_clients(const struct client *clients, enum layout layout, const struct geom *g)
{
	struct geom area;

	assert(g != NULL);

	if (-1 == geom_inset(&area, g, TILE_BORDER,
		FRAME_BORDER + FRAME_SPACING + TILE_MARGIN - TILE_SPACING))
	{
		goto error;
	}

	/* XXX: the arithmetic here is impossible to follow. rework it */
	area.w += TILE_BORDER * 2;
	area.h += TILE_BORDER * 2;

	/* this is overhang for odd number of spacing between tiles */
	/* TODO: check limit. maybe do this using geom_something(), for DRY */
	switch (layout) {
	case LAYOUT_HORIZ: area.w -= TILE_SPACING; break;
	case LAYOUT_VERT:  area.h -= TILE_SPACING; break;
	case LAYOUT_MAX:                           break;
	}

	/* height reserved for tabs */
#if 0
	area.y += 30;
	area.h -= 30;
#endif

	/*
	 * This is worth some explanation.
	 *
	 * The approach here is to progressively divide the area evenly
	 * into n parts, and to place a client at the first 1/nth of that space.
	 * The remaining area is then divided into n - 1 parts, and so on.
	 *
	 * This results in n even-sized windows across the area, but accounts
	 * for fractional skew should n not divide exactly.
	 */
	{
		struct geom new, old;
		const struct client *c;
		unsigned n;

		old = area;

		for (n = client_count(clients), c = clients; n > 0; n--, c = c->next) {
			assert(c != NULL);

			/* XXX: would rather ORDER_NEXT here. order should be an option passed to this function */
			if (n >= 2) {
				layout_split(layout, ORDER_PREV, &new, &old, n);
			}

			/* to account for the double space from layout_split() */
			switch (layout) {
			case LAYOUT_HORIZ: old.w += TILE_SPACING; break;
			case LAYOUT_VERT:  old.h += TILE_SPACING; break;
			case LAYOUT_MAX:                          break;
			}

			if (-1 == win_resize(c->win, &old, TILE_BORDER, TILE_SPACING)) {
				goto error;
			}

			XMapWindow(display, c->win);

			old = new;
		}
	}

	return;

error:

	assert(errno == ERANGE);

	{
		const struct client *c;

		for (c = clients; c != NULL; c = c->next) {
			XUnmapWindow(display, c->win);
		}
	}

	/* TODO: map in "no windows fit" window */
}

