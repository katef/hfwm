#include <assert.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "axis.h"
#include "geom.h"
#include "order.h"
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

struct client *
client_cycle(const struct client *head, const struct client *current_client,
	enum order order)
{
	const struct client *p;

	if (current_client == NULL) {
		return NULL;
	}

	assert(client_find(head, current_client->win));

	switch (order) {
	case ORDER_PREV:
		for (p = head; p->next != NULL; p = p->next) {
			if (p->next == current_client) {
				return (struct client *) p;
			}
		}

		return (struct client *) p;

	case ORDER_NEXT:
		if (current_client->next == NULL) {
			return (struct client *) head;
		}

		return (struct client *) current_client->next;
	}

	return NULL;
}

