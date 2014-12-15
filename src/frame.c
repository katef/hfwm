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
#include "tile.h"

static void
frame_resize(struct frame *p, const struct geom *g)
{
	struct frame *q;

	assert(p != NULL);
	assert(g != NULL);

	p->geom = *g;

	if (-1 == win_resize(p->win, &p->geom,
		FRAME_BORDER, FRAME_SPACING)) {
		XUnmapWindow(display, p->win);
	} else {
		XMapWindow(display, p->win);
	}

	switch (p->type) {
	case FRAME_BRANCH:
		for (q = p->u.children; q != NULL; q = q->next) {
			frame_resize(q, g);
		}
		break;

	case FRAME_LEAF:
		tile_clients(p->u.clients, p->layout, &p->geom);
		break;
	}
}

static void
frame_scale(struct frame *p, const struct ratio *r)
{
	struct frame *q;

	assert(p != NULL);
	assert(r != NULL);

	geom_scale(&p->geom, r);

	if (-1 == win_resize(p->win, &p->geom,
		FRAME_BORDER, FRAME_SPACING)) {
		XUnmapWindow(display, p->win);
	} else {
		XMapWindow(display, p->win);
	}

	switch (p->type) {
	case FRAME_BRANCH:
		for (q = p->u.children; q != NULL; q = q->next) {
			frame_scale(q, r);
		}
		break;

	case FRAME_LEAF:
		tile_clients(p->u.clients, p->layout, &p->geom);
		break;
	}
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

static struct frame *
frame_split(struct frame *old, enum layout layout, enum order order)
{
	struct frame *new;
	struct geom new_geom, old_geom;

	assert(old != NULL);

	old_geom = old->geom;

	layout_split(layout, order, &new_geom, &old_geom, 2);

	/* prospectively match win_create() and win_resize() below */
/* TODO: maybe go ahead and win_create() and win_resize() and undo things on error, instead */
	{
		struct geom tmp;

		if (-1 == geom_inset(&tmp, &new_geom, FRAME_BORDER, FRAME_SPACING)
		 || -1 == geom_inset(&tmp, &old_geom, FRAME_BORDER, FRAME_SPACING))
		{
			assert(errno == ERANGE);
			return old;
		}
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type   = old->type;
	new->parent = old->parent;

	switch (new->type) {
	case FRAME_LEAF:
		new->u.clients  = NULL;
		new->layout     = default_leaf_layout;
		break;

	case FRAME_BRANCH:
		new->u.children = NULL;
		new->layout     = default_branch_layout;
		break;
	}

/* TODO: do after linked list insertion, for event handling */
	new->win = win_create(&new_geom, FRAME_NAME, FRAME_CLASS,
		FRAME_BORDER, FRAME_SPACING);
	if (!new->win) {
		return NULL;
	}

	/* TODO: maybe set Window group (by XSetWMHints() WindowGroupHint) for frames' siblings */
	(void) win_resize(old->win, &old_geom,
		FRAME_BORDER, FRAME_SPACING);

	if (new->type == FRAME_LEAF) {
		new->current_client = NULL;
	}

	old->geom = old_geom;
	new->geom = new_geom;

	switch (order) {
	case ORDER_PREV:
		new->prev = old->prev;
		old->prev = new;
		new->next = old;
		break;

	case ORDER_NEXT:
		new->next = old->next;
		old->next = new;
		new->prev = old;
		break;
	}

	return new;
}

struct frame *
frame_branch(struct frame *p, enum layout layout, enum order order)
{
	struct frame *a, *b;

	assert(p != NULL);

	switch (p->type) {
	case FRAME_BRANCH:
		a = p;

		b = frame_split(a, layout, order);
		if (b == NULL) {
			return NULL;
		}

		if (b == a) {
			return p;
		}

		b->u.children = frame_create_leaf(&b->geom, NULL);
		if (b->u.children == NULL) {
			/* TODO: combine geometry (merge, undoing the split) */
			return NULL;
		}

		return b;

	case FRAME_LEAF:
		a = frame_create_leaf(&p->geom, p->u.clients);
		if (a == NULL) {
			return NULL;
		}

/* XXX: i think creating window before adding it to the list.
maybe need to pass &head after all.
or create window outside? could seperate all window creation from frame.c
*/
		b = frame_split(a, layout, order);
		if (b == NULL) {
			free(a);
			return NULL;
		}

		assert(b->u.clients == NULL);

/* TODO: event to redraw p->win for its new colours etc */

		p->type   = FRAME_BRANCH;
		p->layout = layout;

		switch (order) {
		case ORDER_PREV: p->u.children = b; break;
		case ORDER_NEXT: p->u.children = a; break;
		}

		return b;
	}

	errno = EINVAL;
	return NULL;
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
frame_create_leaf(const struct geom *geom, struct client *clients)
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
	new->parent = NULL;

	return new;
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

int
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
		return 0;
	}

	a = curr->geom;
	b = next->geom;

	if (-1 == layout_redistribute(&b, &a, layout, n)) {
		return -1;
	}

	switch (layout) {
	case LAYOUT_MAX:
		return 0;

	case LAYOUT_HORIZ:
		if (b.w < FRAME_MIN_WIDTH) {
			return 0;
		}
		break;

	case LAYOUT_VERT:
		if (b.h < FRAME_MIN_HEIGHT) {
			return 0;
		}
		break;
	}

	/* TODO: deal with divide by zero
	 * assuming the geom was non-zero beforehand, this can only happen
	 * if n is greater than one dimension, so layout_redistribute
	 * makes a or b occupy the entire space */

	geom_ratio(&ra, &a, &curr->geom);
	geom_ratio(&rb, &b, &next->geom);

	assert(ra.x > 0 && ra.y > 0 && ra.w > 0 && ra.h > 0);
	assert(rb.x > 0 && rb.y > 0 && rb.w > 0 && rb.h > 0);

	frame_scale(curr, &ra);
	frame_scale(next, &rb);

	return 0;
}

struct frame *
frame_find_win(const struct frame *p, Window win)
{
	const struct frame *q;

	assert(p != NULL);

	if (p->win == win) {
		return (struct frame *) p;
	}

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

const char *
frame_type(const struct frame *p)
{
	assert(p != NULL);

	switch (p->type) {
	case FRAME_LEAF:   return "leaf";
	case FRAME_BRANCH: return "branch";
	}

	return "unrecognised";
}

