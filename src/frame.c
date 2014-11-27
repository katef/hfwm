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

static void
frame_scale(struct frame *p, const struct ratio *r)
{
	struct frame *q;
	struct window *w;

	assert(p != NULL);
	assert(r != NULL);

	switch (p->type) {
	case FRAME_BRANCH:
		for (q = p->u.children; q != NULL; q = q->next) {
			frame_scale(q, r);
		}
		break;

	case FRAME_LEAF:
		win_resize(p->win, &p->geom);

		for (w = p->u.windows; w != NULL; w = w->next) {
			/* TODO: win_resize() on each w->win here */
		}
		break;
	}

	geom_scale(&p->geom, r);
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
frame_merge(struct frame *p, enum layout layout, int delta)
{
	struct frame *old;
	enum order order;
	int i;

	assert(p != NULL);

	if (delta == 0) {
		errno = EINVAL;
		return NULL;
	}

	order = delta > 0 ? ORDER_NEXT : ORDER_PREV;

	for (i = 0; i < abs(delta); i++) {
		old = order == ORDER_NEXT ? p->next : p->prev;
		if (old == NULL) {
			return p;
		}

		switch (order) {
		case ORDER_NEXT: p->next = old->next; break;
		case ORDER_PREV: p->prev = old->prev; break;
		}

		layout_merge(order, &p->geom, &old->geom);

		switch (p->type) {
		case FRAME_LEAF:
			switch (order) {
			case ORDER_NEXT:
				win_cat(&p->u.windows, &old->u.windows);
				break;

			case ORDER_PREV:
				win_cat(&old->u.windows, &p->u.windows);
				p->u.windows = old->u.windows;
				break;
			}

			win_resize(p->win, &p->geom); /* XXX: to be done in frame_scale */

			break;

		case FRAME_BRANCH:
			switch (order) {
			case ORDER_NEXT:
				(void) frame_cat(&p->u.children, &old->u.children);
				break;

			case ORDER_PREV:
				(void) frame_cat(&old->u.children, &p->u.children);
				p->u.children = old->u.children;
				break;
			}

/*
TODO: *recursively* update children to resize with their new geometry now *p is different
TODO: would make a frame_scale() for this anyway. call that
don't redraw them - just update sizes
*/
errno = ENOSYS;
return NULL;
			break;
		}

		free(old);
	}

	return p;
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

	if (delta == 0) {
		errno = EINVAL;
		return NULL;
	}

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

void
frame_redistribute(struct frame *p, enum layout layout, int delta, unsigned n)
{
	struct frame *curr, *next;
	enum order order;
	int i;

	assert(p != NULL);

	order = delta > 0 ? ORDER_NEXT : ORDER_PREV;

	curr = p;

	for (i = 0; i < abs(delta); i++) {
		struct geom a, b;
		struct ratio ra, rb;

		next = order == ORDER_NEXT ? p->next : p->prev;

		if (next == NULL) {
			return;
		}

		a = curr->geom;
		b = next->geom;

		layout_redistribute(&a, &b, layout, n);

		geom_ratio(&ra, &a, &curr->geom);
		geom_ratio(&rb, &b, &next->geom);

		frame_scale(curr, &ra);
		frame_scale(next, &rb);

		curr = next;
	}
}

