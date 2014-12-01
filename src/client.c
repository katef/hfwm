#include <assert.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "geom.h"
#include "client.h"

unsigned int
client_count(const struct client *clients)
{
	const struct client *p;
	unsigned int n;

	n = 0;

	for (p = clients; p != NULL; p = p->next) {
		n++;
	}

	return n;
}

void
client_cat(struct client **head, struct client **tail)
{
	struct client **p;

	assert(head != NULL);

	for (p = head; *p != NULL; p = &(*p)->next)
		;

	*p    = *tail;
	*tail = NULL;
}

struct client *
client_find(const struct client *head, Window win)
{
	const struct client *p;

	for (p = head; p != NULL; p = p->next) {
		if (p->win == win) {
			return (struct client *) p;
		}
	}

	return NULL;
}

struct client *
client_add(struct client **head, Window win)
{
	struct client *new;

	assert(head != NULL);
	assert(!client_find(*head, win));

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->win = win;

	new->next = *head;
	*head = new;

	return new;
}

void
client_remove(struct client **head, Window win)
{
	struct client **p;

	assert(head != NULL);

	for (p = head; *p != NULL; p = &(*p)->next) {
		if ((*p)->win == win) {
			struct client *next;

			next = (*p)->next;
			free(*p);
			*p = next;

			return;
		}
	}
}

