#ifndef LAYOUT_H
#define LAYOUT_H

enum layout {
	LAYOUT_HORIZ,
	LAYOUT_VERT,
	LAYOUT_MAX
};

enum order {
	ORDER_PREV,
	ORDER_NEXT
};

struct geom {
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
};

enum layout
layout_lookup(const char *name);

void
layout_split(enum layout layout, enum order order, struct geom *new, struct geom *old);

#endif

