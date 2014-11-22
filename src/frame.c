#include <assert.h>
#include <stdlib.h>

#include "frame.h"

struct window {
	/* TODO: placeholder */
	struct window *next;
};

struct frame {
	struct frame *next;   /* list of sibling frames */
	struct frame *child;  /* list of zero or more children by .next */
	struct frame *parent; /* pointer up */

	struct window *windows; /* list of windows in this frame */

	enum layout frame_layout;  /* layout for .child list */
	enum layout window_layout; /* layout for .window list */

	unsigned x;
	unsigned y;
	unsigned h;
	unsigned w;
};

struct frame *current_frame;

struct frame *
frame_prepend(struct frame **head)
{
	struct frame *new;

	assert(head != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->frame_layout  = LAYOUT_HORIZ; /* TODO: configurable default */
	new->window_layout = LAYOUT_MAX;   /* TODO: configurable default */

	/* TODO: calculate from parents and siblings, and redistribute siblings */
	/* TODO: to find siblings, go up to parent, and search lists */
	new->x = 0;
	new->y = 0;
	new->h = 0;
	new->w = 0;

	new->windows = NULL;

	new->child  = NULL;
	new->parent = (*head)->parent;

	*head = new;
	new->next = *head;

	return new;
}

struct frame *
frame_append(struct frame **head)
{
	struct frame **p;

	assert(head != NULL);

	for (p = head; (*p)->next != NULL; p = &(*p)->next)
		;

	return frame_prepend(p);
}

