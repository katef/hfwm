/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "axis.h"
#include "geom.h"
#include "order.h"
#include "layout.h"
#include "frame.h"
#include "client.h"
#include "tile.h"
#include "win.h"

static int
tile_empty(const struct geom *area)
{
	assert(area != NULL);

	/* TODO: map in an "empty" window */
	(void) area;

	return 0;
}

static int
tile_max(const struct client *clients, const struct geom *area,
	const struct client *curr)
{
	const struct client *c;

	assert(clients != NULL);
	assert(area != NULL);
	assert(curr != NULL);

	/* height reserved for tabs */
	/* TODO: map in "tabs" window, if present */
#if 0
	area->y += 30;
	area->h -= 30;
#endif

	for (c = clients; c != NULL; c = c->next) {
		if (c == curr) {
			continue;
		}

		XUnmapWindow(display, c->win);
	}

	if (-1 == win_resize(curr->win, area, TILE_BORDER, TILE_SPACING)) {
		return -1;
	}

	/* TODO */
	XMapWindow(display, curr->win);

	return 0;
}

static int
tile_axis(const struct client *clients, const struct geom *area, enum axis axis)
{
	struct geom g;
	struct geom new, old;
	const struct client *c;
	unsigned n;

	assert(clients != NULL);
	assert(area != NULL);

	g = *area;

	/* XXX: the arithmetic here is impossible to follow. rework it */
	g.w += TILE_BORDER * 2;
	g.h += TILE_BORDER * 2;

	/* this is overhang for odd number of spacing between tiles */
	/* TODO: check limit. maybe do this using geom_something(), for DRY */
	switch (axis) {
	case AXIS_HORIZ: g.w -= TILE_SPACING; break;
	case AXIS_VERT:  g.h -= TILE_SPACING; break;
	}

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

	old = g;

	for (n = client_count(clients), c = clients; n > 0; n--, c = c->next) {
		assert(c != NULL);

		/* XXX: would rather ORDER_NEXT here. order should be an option passed to this function */
		if (n >= 2) {
			layout_split((enum layout) axis, ORDER_PREV, &new, &old, n);
		}

		/* to account for the double space from layout_split() */
		switch (axis) {
		case AXIS_HORIZ: old.w += TILE_SPACING; break;
		case AXIS_VERT:  old.h += TILE_SPACING; break;
		}

		if (-1 == win_resize(c->win, &old, TILE_BORDER, TILE_SPACING)) {
			return -1;
		}

		XMapWindow(display, c->win);

		old = new;
	}

	return 0;
}

void
tile_clients(const struct client *clients, enum layout layout, const struct geom *g,
	const struct client *curr)
{
	struct geom area;
	int r;

	assert(g != NULL);
	assert(curr != NULL || clients == NULL);

	if (-1 == geom_inset(&area, g, TILE_BORDER,
		FRAME_BORDER + FRAME_SPACING + TILE_MARGIN - TILE_SPACING))
	{
		goto error;
	}

	if (clients == NULL) {
		r = tile_empty(&area);
	} else if (layout == LAYOUT_MAX) {
		r = tile_max(clients, &area, curr);
	} else {
		r = tile_axis(clients, &area, (enum axis) layout);
	}

	if (r == -1) {
		goto error;
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

	/* TODO: map in a "no windows fit" window */
}

