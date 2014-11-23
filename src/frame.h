#ifndef FRAME_H
#define FRAME_H

struct window;
struct geom;

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

extern struct frame *current_frame;

struct frame *
frame_create(void);

struct frame *
frame_split(struct frame *old, enum order order);

struct frame *
frame_sibling(struct frame *curr, int delta);

#endif

