#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include "geom.h"
#include "order.h"
#include "layout.h"

enum layout default_branch_layout = LAYOUT_HORIZ; /* TODO: configurable default */
enum layout default_leaf_layout   = LAYOUT_MAX;   /* TODO: configurable default */

enum layout
layout_lookup(const char *s)
{
	size_t i;

	struct {
		const char *name;
		enum layout layout;
	} a[] = {
		{ "horiz", LAYOUT_HORIZ },
		{ "vert",  LAYOUT_VERT  },
		{ "max",   LAYOUT_MAX   }
	};

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, s)) {
			return a[i].layout;
		}
	}

	return -1;
}

enum layout
layout_cycle(enum layout l, int delta)
{
	return (l + delta) % (LAYOUT_COUNT + 1);
}

void
layout_split(enum layout layout, enum order order, struct geom *new, struct geom *old,
	unsigned int n)
{
	unsigned int orig;

	assert(new != NULL);
	assert(old != NULL);
	assert(n >= 2);

	switch (layout) {
	case LAYOUT_HORIZ:
		new->w = (orig = old->w, orig - (old->w /= n));
		new->h = old->h;
		new->x = old->x;
		new->y = old->y;

		switch (order) {
		case ORDER_NEXT: new->x += new->w; break;
		case ORDER_PREV: old->x += new->w; break;
		}
		break;

	case LAYOUT_VERT:
		new->w = old->w;
		new->h = (orig = old->h, orig - (old->h /= n));
		new->x = old->x;
		new->y = old->y;

		switch (order) {
		case ORDER_NEXT: new->y += new->h; break;
		case ORDER_PREV: old->y += new->h; break;
		}
		break;

	case LAYOUT_MAX:
		*new = *old;
		break;
	}
}

void
layout_merge(enum order order, struct geom *dst, struct geom *src)
{
	assert(dst != NULL);
	assert(src != NULL);

	dst->w += src->w;
	dst->h += src->h;

	switch (order) {
	case ORDER_PREV:
		dst->x = src->x;
		dst->y = src->y;
		break;

	case ORDER_NEXT:
		break;
	}
}

int
layout_redistribute(struct geom *a, struct geom *b, enum layout layout, unsigned n)
{
	assert(a != NULL);
	assert(b != NULL);

	switch (layout) {
	case LAYOUT_MAX:
		break;

	case LAYOUT_HORIZ:
		if (n > b->w) {
			errno = ERANGE;
			return -1;
		}

		a->w += n;
		b->w -= n;
		break;

	case LAYOUT_VERT:
		if (n > b->h) {
			errno = ERANGE;
			return -1;
		}

		a->h += n;
		b->h -= n;
		break;
	}

	return 0;
}

