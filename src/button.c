#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include "button.h"

struct button *buttons[BUTTON_MAX];

struct button *
button_find(int button, int mod)
{
	const struct button *head, *p;

	if (button < 0 || button > sizeof buttons / sizeof *buttons) {
		errno = EINVAL;
		return NULL;
	}

	head = buttons[button];

	for (p = head; p != NULL; p = p->next) {
		if (p->mod == mod) {
			return (struct button *) p;
		}
	}

	return NULL;
}

struct button *
button_provision(int button, int mod)
{
	struct button **head, *p, *new;

	if (button < 0 || button > sizeof buttons / sizeof *buttons) {
		errno = EINVAL;
		return NULL;
	}

	p = button_find(button, mod);
	if (p != NULL) {
		return p;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->mod   = mod;
	new->chain = NULL;

	head = &buttons[button];

	new->next = *head;
	*head = new;

	return new;
}

