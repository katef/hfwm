#include <assert.h>
#include <stddef.h>

#include <X11/X.h>

#include "current.h"
#include "order.h"
#include "layout.h"
#include "geom.h"
#include "client.h"
#include "event.h"
#include "frame.h"

struct frame *current_frame;

void
set_current_frame(struct frame *p)
{
	assert(p != NULL);
	assert(current_frame != NULL);

	if (current_frame == p) {
		return;
	}

	if (current_frame->type == FRAME_LEAF && current_frame->current_client != NULL) {
		event_issue(EVENT_DIOPTRE, "blur client %p",
			(void *) current_frame->current_client->win);
	}

	event_issue(EVENT_DIOPTRE, "blur %s %p",
		frame_type(current_frame),
		(void *) current_frame->win);

	current_frame = p;

	event_issue(EVENT_DIOPTRE, "focus %s %p",
		frame_type(current_frame),
		(void *) current_frame->win);

	if (current_frame->type == FRAME_LEAF && current_frame->current_client != NULL) {
		event_issue(EVENT_DIOPTRE, "focus client %p",
			(void *) current_frame->current_client->win);
	}
}

void
set_current_client(struct frame *p, struct client *q)
{
	assert(p != NULL);

	if (p->current_client == q) {
		return;
	}

	if (p->type == FRAME_LEAF && p->current_client != NULL) {
		event_issue(EVENT_DIOPTRE, "blur client %p",
			(void *) p->current_client->win);
	}

	p->current_client = q;

	if (current_frame->type == FRAME_LEAF && p->current_client != NULL) {
		event_issue(EVENT_DIOPTRE, "focus client %p",
			(void *) current_frame->current_client->win);
	}
}

