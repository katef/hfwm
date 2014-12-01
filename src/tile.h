#ifndef TILE_H
#define TILE_H

#define TILE_BORDER  10
#define TILE_SPACING 10
#define TILE_MARGIN   4

struct geom;
struct client;

int
tile_resize(const struct frame *p);

#endif

