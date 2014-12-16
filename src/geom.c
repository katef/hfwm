#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "axis.h"
#include "geom.h"

int
geom_inset(struct geom *in, const struct geom *g,
	unsigned int bw, int spacing)
{
	assert(in != NULL);
	assert(g != NULL);

	in->x = g->x + spacing;
	in->y = g->y + spacing;

	in->w = g->w - bw * 2 - spacing * 2;
	in->h = g->h - bw * 2 - spacing * 2;

	if (g->w < in->w || g->h < in->h) {
		errno = ERANGE;
		return -1;
	}

	return 0;
}

void
geom_ratio(struct ratio *r, const struct geom *num, const struct geom *denom)
{
	assert(r != NULL);
	assert(num != NULL);
	assert(denom != NULL);

	assert(denom->x != 0 && denom->y != 0);
	assert(denom->w != 0 && denom->h != 0);

	r->x = num->x / (double) denom->x;
	r->y = num->y / (double) denom->y;

	r->w = num->w / (double) denom->w;
	r->h = num->h / (double) denom->h;
}

void
geom_scale(struct geom *g, const struct ratio *r)
{
	assert(g != NULL);
	assert(r != NULL);

	/* TODO: surely rounding errors galore */

	g->x *= r->x;
	g->y *= r->y;

	g->w *= r->w;
	g->h *= r->h;
}

int
geom_move(struct geom *g, enum axis axis, int n)
{
	unsigned int *m;

	assert(g != NULL);

	switch (axis) {
	case AXIS_HORIZ: m = &g->x; break;
	case AXIS_VERT:  m = &g->y; break;
	}

	if (n > 0 && UINT_MAX - *m < (unsigned) n) {
		errno = ERANGE;
		return -1;
	}

	if (n < 0 && *m < (unsigned) -n) {
		errno = ERANGE;
		return -1;
	}

	*m += n;

	return 0;
}

