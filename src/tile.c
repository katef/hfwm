#include <assert.h>

#include <X11/Xlib.h>

#include "geom.h"
#include "order.h"
#include "layout.h"
#include "frame.h"
#include "client.h"
#include "tile.h"
#include "win.h"

int
tile_resize(const struct frame *p)
{
	struct geom area;

	assert(p != NULL);

	/* XXX: should not be neccessary; why can't &p->geom be passed const to geom_inner? */
	area = p->geom;

	if (-1 == geom_inner(&area, &area, FRAME_BORDER + FRAME_SPACING, TILE_MARGIN - TILE_SPACING)) {
		return -1;
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
		struct client *c;
		unsigned n;

		old = area;

		for (n = client_count(p->u.clients), c = p->u.clients; n > 0; n--, c = c->next) {
			assert(c != NULL);

			/* XXX: would rather ORDER_NEXT here. order should be an option passed to this function */
			if (n >= 2) {
				layout_split(p->layout, ORDER_PREV, &new, &old, n);
			}

			win_resize(c->win, &old, TILE_BORDER, TILE_SPACING);

			old = new;
		}
	}

	return 0;
}

