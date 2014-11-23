#include <assert.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "frame.h"

enum layout default_frame_layout  = LAYOUT_HORIZ; /* TODO: configurable default */
enum layout default_window_layout = LAYOUT_MAX;   /* TODO: configurable default */

struct window {
	/* TODO: placeholder */
	struct window *next;
};

struct geom {
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
};

struct frame {
	struct frame *next;   /* list of sibling frames */
	struct frame *child;  /* list of zero or more children by .next */
	struct frame *parent; /* pointer up */

	struct window *windows; /* list of windows in this frame */

	enum layout frame_layout;  /* layout for .child list */
	enum layout window_layout; /* layout for .window list */

	struct geom geom;
};

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

static void
layout_split(enum layout layout, struct geom *new, struct geom *old)
{
	unsigned int orig;

	assert(new != NULL);
	assert(old != NULL);

	switch (layout) {
	case LAYOUT_HORIZ:
		new->w = (orig = old->w, orig - (old->w /= 2));
		new->h = old->h;
		new->x = old->x + new->w;
		new->y = old->y;
		break;

	case LAYOUT_VERT:
		new->w = old->w;
		new->h = (orig = old->h, orig - (old->h /= 2));
		new->x = old->x;
		new->y = old->y + new->h;
		break;

	case LAYOUT_MAX:
		*new = *old;
		break;
	}
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

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->frame_layout  = default_frame_layout;
	new->window_layout = default_window_layout;

	new->windows = NULL;

	new->next   = NULL;
	new->child  = NULL;
	new->parent = NULL;

	new->geom.x = x;
	new->geom.y = y;
	new->geom.w = w;
	new->geom.h = h;

	return new;
}

static struct frame *
frame_split(struct frame **p)
{
	struct frame *old, *new;

	assert(p != NULL);
	assert(*p != NULL);

	old = *p;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->frame_layout  = default_frame_layout;
	new->window_layout = default_window_layout;

	new->windows = NULL;

	new->child  = NULL;
	new->parent = old->parent;

	layout_split(new->frame_layout, &new->geom, &old->geom);

rectangle(XFillRectangle, &old->geom, "#556666");
rectangle(XDrawRectangle, &old->geom, "#222222");
rectangle(XFillRectangle, &new->geom, "#555566");
rectangle(XDrawRectangle, &new->geom, "#222222");

	*p = new;
	new->next = *p;

	current_frame = old;

	return new;
}

struct frame *
frame_prepend(struct frame **head)
{
	assert(head != NULL);
	assert(*head != NULL);

	return frame_split(head);
}

struct frame *
frame_append(struct frame **head)
{
	struct frame **p;

	assert(head != NULL);
	assert(*head != NULL);

	for (p = head; (*p)->next != NULL; p = &(*p)->next)
		;

	return frame_split(p);
}

