#include <assert.h>
#include <stdlib.h>

#include "cmd.h"
#include "chain.h"

struct chain {
	char *const *argv;
	struct chain *next;
};

struct chain *
chain_append(struct chain **head, char *const *argv)
{
	struct chain **p, *new;

	assert(argv != NULL);

	for (p = head; *p != NULL; p = &(*p)->next)
		;

	new = malloc(sizeof **p);
	if (new == NULL) {
		return NULL;
	}

	new->argv = argv;
	new->next = NULL;

	*p = new;

	return new;
}

int
chain_dispatch(const struct chain *chain)
{
	const struct chain *p;

	for (p = chain; p != NULL; p = p->next) {
		if (-1 == cmd_dispatch(p->argv)) {
			return -1;
		}
	}

	return 0;
}

void
chain_free(struct chain *chain)
{
	struct chain *p, *next;

	for (p = chain; p != NULL; p = next) {
		next = p->next;
		free((char **) p->argv);
		free(p);
	}
}

