/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TILE_H
#define TILE_H

#define TILE_BORDER  10
#define TILE_SPACING 10
#define TILE_MARGIN   4

struct geom;
struct client;

void
tile_clients(const struct client *clients, enum layout layout, const struct geom *g,
	const struct client *curr);

#endif

