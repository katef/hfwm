#ifndef CLIENT_H
#define CLIENT_H

struct client {
	Window win;
	struct client *next;
};

unsigned int
client_count(const struct client *clients);

void
client_cat(struct client **head, struct client **tail);

struct client *
client_find(const struct client *head, Window win);

struct client *
client_prev(const struct client *head, const struct client *c);

struct client *
client_add(struct client **head, Window win);

void
client_remove(struct client **head, Window win);

struct client *
client_cycle(const struct client *head, const struct client *current_client,
	enum order order);

#endif

