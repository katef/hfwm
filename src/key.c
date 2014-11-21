#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include "key.h"
#include "cmd.h"

struct key {
	unsigned int keycode;
	int mod;
	char **argv;
	struct key *next;
};

struct key *keys;

static struct key *
key_find(struct key *head, unsigned int keycode, int mod)
{
	struct key *p;

	for (p = head; p != NULL; p = p->next) {
		if (p->keycode == keycode && p->mod == mod) {
			return p;
		}
	}

	return NULL;
}

static int
key_add(struct key **head, unsigned int keycode, int mod, char *argv[])
{
	struct key *new;

	assert(head != NULL);
	assert(argv != NULL);

	if (key_find(keys, keycode, mod)) {
		errno = EEXIST;
		return -1;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return -1;
	}

/* TODO: duplicate argv */

	new->keycode = keycode;
	new->mod     = mod;
	new->argv    = argv;

	new->next = *head;
	*head     = new;

	return 0;
}

int
key_bind(unsigned int keycode, int mod, char *argv[])
{
	assert(argv != NULL);

	return key_add(&keys, keycode, mod, argv);
}

int
key_dispatch(unsigned int keycode, int mod)
{
	struct key *key;

	key = key_find(keys, keycode, mod);
	if (key == NULL) {
		errno = ENOENT;
		return -1;
	}

	return cmd_dispatch(key->argv);
}

