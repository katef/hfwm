#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "layout.h"
#include "frame.h"

enum layout default_frame_layout  = LAYOUT_HORIZ; /* TODO: configurable default */
enum layout default_window_layout = LAYOUT_MAX;   /* TODO: configurable default */

struct window {
	/* TODO: placeholder */
	struct window *next;
};

struct frame {
	struct frame *prev;     /* list of sibling frames */
	struct frame *next;     /* list of sibling frames */
	struct frame *parent;   /* pointer up */
	struct frame *children; /* list of zero or more children by .prev/.next */

	struct window *windows; /* list of windows in this frame */

	enum layout frame_layout;  /* layout for .children list */
	enum layout window_layout; /* layout for .window list */

	struct geom geom;
};

/* TODO: maybe lives in cmd.c */
struct frame *current_frame;

extern Display *display; /* XXX */
extern Window root; /* XXX */

/* XXX: debugging stuff; no error checking */
static void
rectangle(int (*f)(Display *, Drawable, GC, int, int, unsigned int, unsigned int),
	struct geom *geom, const char *colour)
{
	Colormap cm;
	XColor col;
	GC gc;

	assert(f != NULL);
	assert(geom != NULL);
	assert(colour != NULL);

	cm = DefaultColormap(display, 0);

	gc = XCreateGC(display, root, 0, 0);
	XParseColor(display, cm, colour, &col);
	XAllocColor(display, cm, &col);
	XSetForeground(display, gc, col.pixel);

	f(display, root, gc, geom->x, geom->y, geom->w, geom->h);

	XFlush(display);
}

struct frame *
frame_create(void)
{
	struct frame *new;
	int x, y;
	unsigned int w, h;
	Window rr;
	unsigned int bw;
	unsigned int depth;

	if (!XGetGeometry(display, root, &rr, &x, &y, &w, &h, &bw, &depth)) {
		return NULL;
	}

	if (x < 0 || y < 0) {
		errno = EINVAL;
		return NULL;
	}

	if (w == 0 || h == 0) {
		errno = EINVAL;
		return NULL;
	}

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

	new->geom.x = x;
	new->geom.y = y;
	new->geom.w = w;
	new->geom.h = h;

	return new;
}

struct frame *
frame_split(struct frame *old, enum order order)
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

	layout_split(new->frame_layout, order, &new->geom, &old->geom);

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
frame_sibling(struct frame *curr, int delta)
{
	struct frame *next;
	int i;

	assert(curr != NULL);

	if (delta == 0) {
		errno = EINVAL;
		return NULL;
	}

rectangle(XFillRectangle, &curr->geom, "#161615");
rectangle(XDrawRectangle, &curr->geom, "#121212");

	for (i = 0; i < abs(delta); i++) {
		next = delta > 0 ? curr->next : curr->prev;

		if (next == NULL) {
			return NULL;
		}

		curr = next;
	}

rectangle(XFillRectangle, &curr->geom, "#665555");
rectangle(XDrawRectangle, &curr->geom, "#222222");

	return curr;
}

