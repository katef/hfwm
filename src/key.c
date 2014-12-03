#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include "key.h"

static struct key *keys;

struct key *
key_find(unsigned int keycode, int mod)
{
	const struct key *head, *p;

	head = keys;

	for (p = head; p != NULL; p = p->next) {
		if (p->keycode == keycode && p->mod == mod) {
			return (struct key *) p;
		}
	}

	return NULL;
}

struct key *
key_provision(unsigned int keycode, int mod)
{
	struct key **head, *p, *new;

	p = key_find(keycode, mod);
	if (p != NULL) {
		return p;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->keycode = keycode;
	new->mod     = mod;
	new->chain   = NULL;

	head = &keys;

	new->next = *head;
	*head     = new;

	return new;
}

