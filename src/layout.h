#ifndef LAYOUT_H
#define LAYOUT_H

struct geom;

enum layout {
	LAYOUT_HORIZ,
	LAYOUT_VERT,
	LAYOUT_MAX
};

#define LAYOUT_COUNT 3

enum layout
layout_lookup(const char *name);

enum layout
layout_cycle(enum layout l, int delta);

void
layout_split(enum layout layout, enum order order, struct geom *new, struct geom *old);

#endif

