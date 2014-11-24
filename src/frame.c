#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "geom.h"
#include "order.h"
#include "layout.h"
#include "frame.h"
#include "main.h"
#include "win.h"

enum layout default_frame_layout  = LAYOUT_HORIZ; /* TODO: configurable default */
enum layout default_window_layout = LAYOUT_MAX;   /* TODO: configurable default */

/* TODO: maybe lives in cmd.c */
struct frame *current_frame;

struct frame *
frame_create(const struct geom *g)
{
	struct frame *new;

	assert(g != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->frame_layout  = default_frame_layout;
	new->window_layout = default_window_layout;

	new->windows  = NULL;

	new->prev     = NULL;
	new->next     = NULL;
	new->parent   = NULL;
	new->children = NULL;

	new->geom     = *g;

	return new;
}

struct frame *
frame_split(struct frame *old, enum layout layout, enum order order)
{
	struct frame *new;

	assert(old != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->frame_layout  = default_frame_layout;
	new->window_layout = default_window_layout;

	new->windows = NULL;

	new->parent   = old->parent;
	new->children = NULL;

	layout_split(layout, order, &new->geom, &old->geom);

rectangle(XFillRectangle, &old->geom, "#556666");
rectangle(XDrawRectangle, &old->geom, "#222222");
rectangle(XFillRectangle, &new->geom, "#555566");
rectangle(XDrawRectangle, &new->geom, "#222222");

	switch (order) {
	case ORDER_NEXT:
		new->next = old->next;
		old->next = new;
		new->prev = old;
		break;

	case ORDER_PREV:
		new->prev = old->prev;
		old->prev = new;
		new->next = old;
		break;
	}

	return new;
}

struct frame *
frame_focus(struct frame *curr, enum rel rel, int delta)
{
	struct frame *next;
	int i;

	assert(curr != NULL);

rectangle(XDrawRectangle, &curr->geom, "#662222");

	for (i = 0; i < abs(delta); i++) {
		switch (rel) {
		case REL_SIBLING: next = delta > 0 ? curr->next     : curr->prev;   break;
		case REL_LINEAGE: next = delta > 0 ? curr->children : curr->parent; break;
		}

		if (next == NULL) {
			return NULL;
		}

		curr = next;
	}

rectangle(XDrawRectangle, &curr->geom, "#222222");

	return curr;
}

