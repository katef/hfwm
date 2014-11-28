#ifndef GEOM_H
#define GEOM_H

struct geom {
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
};

struct ratio {
	double x;
	double y;
	double w;
	double h;
};

int
geom_inner(struct geom *in, const struct geom *g, unsigned int bw);

void
geom_ratio(struct ratio *r, const struct geom *old, const struct geom *new);

void
geom_scale(struct geom *g, const struct ratio *r);

#endif

