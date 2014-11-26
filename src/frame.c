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

/* TODO: maybe lives in cmd.c */
struct frame *current_frame;

struct frame *
frame_split(struct frame *old, enum layout layout, enum order order)
{
	struct frame *new;

	assert(old != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type   = old->type;
	new->parent = old->parent;

	switch (new->type) {
	case FRAME_LEAF:
		new->u.windows = NULL;
		new->layout    = default_leaf_layout;
		break;

	case FRAME_BRANCH:
		new->u.children = NULL;
		new->layout     = default_branch_layout;
		break;
	}

	layout_split(layout, order, &new->geom, &old->geom);

	if (new->type == FRAME_LEAF) {
		new->win = win_create(&new->geom, "hfwm");
		win_resize(old->win, &new->geom);
	}

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

void
frame_cat(struct frame **head, struct frame **tail)
{
	struct frame **p;

	assert(head != NULL);

	for (p = head; *p != NULL; p = &(*p)->next)
		;

	*p    = *tail;
	*tail = NULL;
}

struct frame *
frame_create_leaf(struct frame *parent, const struct geom *geom,
	struct window *windows)
{
	struct frame *new;

	assert(geom != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->win = win_create(geom, "hfwm");
	if (!new->win) {
		free(new);
		return NULL;
	}

	new->type      = FRAME_LEAF;
	new->u.windows = windows;

	new->layout = default_leaf_layout;
	new->geom   = *geom;

	new->prev   = NULL;
	new->next   = NULL;
	new->parent = parent;

	return new;
}

struct frame *
frame_branch_leaf(struct frame *old, enum layout layout, enum order order,
	struct window *windows)
{
	struct frame *a, *b;

	assert(old != NULL);
	assert(old->type == FRAME_LEAF);

	a = frame_create_leaf(old, &old->geom, old->u.windows);
	if (a == NULL) {
		return NULL;
	}

	b = frame_split(a, layout, order);
	if (b == NULL) {
		free(a);
		return NULL;
	}

	b->u.windows = windows;

	old->type       = FRAME_BRANCH;
	old->layout     = layout;
	old->u.children = a; /* or b */

	return b;
}

struct frame *
frame_focus(struct frame *curr, enum rel rel, int delta)
{
	struct frame *next;
	int i;

	assert(curr != NULL);

	for (i = 0; i < abs(delta); i++) {
		switch (rel) {
		case REL_SIBLING: next = delta > 0 ? curr->next       : curr->prev;   break;
		case REL_LINEAGE: next = delta > 0 ? curr->u.children : curr->parent; break;
		}

		if (curr->type != FRAME_BRANCH && next == curr->u.children) {
			return curr;
		}

		if (next == NULL) {
			return curr;
		}

		curr = next;
	}

	return curr;
}

