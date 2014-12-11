#ifndef TILE_H
#define TILE_H

#define TILE_BORDER  10
#define TILE_SPACING 10
#define TILE_MARGIN   4

struct geom;
struct client;

int
tile_clients(const struct client *clients, enum layout layout, const struct geom *g);

#endif

