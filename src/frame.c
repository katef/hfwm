#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "geom.h"
#include "order.h"
#include "layout.h"
#include "frame.h"
#include "win.h"
#include "client.h"
#include "current.h"

static void
frame_resize(struct frame *p, const struct geom *g)
{
	struct frame *q;
	struct client *c;

	assert(p != NULL);
	assert(g != NULL);

	switch (p->type) {
	case FRAME_BRANCH:
		for (q = p->u.children; q != NULL; q = q->next) {
			frame_resize(q, g);
		}
		break;

	case FRAME_LEAF:
		(void) win_resize(p->win, &p->geom,
			FRAME_BORDER, FRAME_SPACING);

		for (c = p->u.clients; c != NULL; c = c->next) {
			/* TODO: win_resize() on each c->win here */
		}
		break;
	}

	p->geom = *g;
}

static void
frame_scale(struct frame *p, const struct ratio *r)
{
	struct frame *q;
	struct client *c;

	assert(p != NULL);
	assert(r != NULL);

	switch (p->type) {
	case FRAME_BRANCH:
		for (q = p->u.children; q != NULL; q = q->next) {
			frame_scale(q, r);
		}
		break;

	case FRAME_LEAF:
		(void) win_resize(p->win, &p->geom,
			FRAME_BORDER, FRAME_SPACING);

		for (c = p->u.clients; c != NULL; c = c->next) {
			/* TODO: win_resize() on each c->win here */
		}
		break;
	}

	geom_scale(&p->geom, r);
}

enum rel
rel_lookup(const char *s)
{
	size_t i;

	struct {
		const char *name;
		enum rel rel;
	} a[] = {
		{ "sibling", REL_SIBLING },
		{ "lineage", REL_LINEAGE }
	};

	if (s == NULL) {
		return REL_SIBLING;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].name)) {
			return a[i].rel;
		}
	}

	return -1;
}

struct frame *
frame_top(void)
{
	const struct frame *p;

	for (p = current_frame; p->parent != NULL; p = p->parent)
		;

	return (struct frame *) p;
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
		new->u.clients = NULL;
		new->layout    = default_leaf_layout;
		break;

	case FRAME_BRANCH:
		new->u.children = NULL;
		new->layout     = default_branch_layout;
		break;
	}

	layout_split(layout, order, &new->geom, &old->geom, 2);

	if (new->type == FRAME_LEAF) {
		new->current_client = NULL;

		new->win = win_create(&new->geom, FRAME_NAME, FRAME_CLASS,
			FRAME_BORDER, FRAME_SPACING);
		if (!new->win) {
			return NULL;
		}

		/* TODO: maybe set Window group (by XSetWMHints() WindowGroupHint) for frames' siblings */
		(void) win_resize(old->win, &old->geom,
			FRAME_BORDER, FRAME_SPACING);
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
frame_merge(struct frame *p, enum layout layout, enum order order)
{
	struct frame *old;

	assert(p != NULL);

	(void) layout;

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
			client_cat(&p->u.clients, &old->u.clients);
			break;

		case ORDER_PREV:
			client_cat(&old->u.clients, &p->u.clients);
			p->u.clients = old->u.clients;
			break;
		}
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
		break;
	}

	frame_resize(p, &p->geom);

	win_destroy(old->win);

	free(old);

	return p;
}

struct frame *
frame_create_leaf(struct frame *parent, const struct geom *geom,
	struct client *clients)
{
	struct frame *new;

	assert(geom != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->win = win_create(geom, FRAME_NAME, FRAME_CLASS,
		FRAME_BORDER, FRAME_SPACING);
	if (!new->win) {
		free(new);
		return NULL;
	}

	new->current_client = NULL;

	new->type      = FRAME_LEAF;
	new->u.clients = clients;

	new->layout = default_leaf_layout;
	new->geom   = *geom;

	new->prev   = NULL;
	new->next   = NULL;
	new->parent = parent;

	return new;
}

struct frame *
frame_branch_leaf(struct frame *old, enum layout layout, enum order order,
	struct client *clients)
{
	struct frame *a, *b;

	assert(old != NULL);
	assert(old->type == FRAME_LEAF);

	a = frame_create_leaf(old, &old->geom, old->u.clients);
	if (a == NULL) {
		return NULL;
	}

	b = frame_split(a, layout, order);
	if (b == NULL) {
		free(a);
		return NULL;
	}

	b->u.clients = clients;

	old->type       = FRAME_BRANCH;
	old->layout     = layout;
	old->u.children = a; /* or b */

	/* TODO: optimisation; transplant this to one of the leaves, instead of creating a new Window */
	win_destroy(old->win);

	return b;
}

struct frame *
frame_focus(struct frame *curr, enum rel rel, enum order order)
{
	struct frame *next;

	assert(curr != NULL);

	switch (order) {
	case ORDER_NEXT:
		if (curr->type != FRAME_BRANCH && rel == REL_LINEAGE) {
			return curr;
		}

		switch (rel) {
		case REL_SIBLING: next = curr->next;       break;
		case REL_LINEAGE: next = curr->u.children; break;
		}

		break;

	case ORDER_PREV:
		switch (rel) {
		case REL_SIBLING: next = curr->prev;       break;
		case REL_LINEAGE: next = curr->parent;     break;
		}

		break;
	}

	if (next == NULL) {
		return curr;
	}

	return next;
}

void
frame_redistribute(struct frame *p, enum layout layout, enum order order, unsigned n)
{
	struct frame *curr, *next;
	struct ratio ra, rb;
	struct geom a, b;

	assert(p != NULL);

	curr = p;

	switch (order) {
	case ORDER_NEXT: next = p->next; break;
	case ORDER_PREV: next = p->prev; break;
	}

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
}

struct frame *
frame_find_win(const struct frame *p, Window win)
{
	const struct frame *q;

	assert(p != NULL);

	switch (p->type) {
	case FRAME_BRANCH:
		for (q = p->u.children; q != NULL; q = q->next) {
			struct frame *r;

			r = frame_find_win(q, win);
			if (r != NULL) {
				return r;
			}
		}
		break;

	case FRAME_LEAF:
		if (p->win == win) {
			return (struct frame *) p;
		}
		break;
	}

	return NULL;
}

struct frame *
frame_find_client(const struct frame *p, Window win)
{
	const struct frame *q;

	assert(p != NULL);

	switch (p->type) {
	case FRAME_BRANCH:
		for (q = p->u.children; q != NULL; q = q->next) {
			struct frame *r;

			r = frame_find_client(q, win);
			if (r != NULL) {
				return r;
			}
		}
		break;

	case FRAME_LEAF:
		if (client_find(p->u.clients, win)) {
			return (struct frame *) p;
		}
		break;
	}

	return NULL;
}

