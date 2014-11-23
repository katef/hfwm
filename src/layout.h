#ifndef LAYOUT_H
#define LAYOUT_H

enum layout {
	LAYOUT_HORIZ,
	LAYOUT_VERT,
	LAYOUT_MAX
};

struct geom {
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
};

void
layout_split(enum layout layout, struct geom *new, struct geom *old);

#endif

