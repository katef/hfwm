#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include "button.h"
#include "args.h"
#include "cmd.h"

struct button {
	int mod;
	char **argv;
	struct button *next;
};

struct button *buttons[BUTTON_MAX];

static struct button *
button_find(struct button *head, int button, int mod)
{
	struct button *p;

	for (p = head; p != NULL; p = p->next) {
		if (p->mod == mod) {
			return p;
		}
	}

	return NULL;
}

static int
button_add(struct button **head, int button, int mod, char *argv[])
{
	struct button *new;

	assert(head != NULL);
	assert(argv != NULL);

	if (button < 0 || button > sizeof buttons / sizeof *buttons) {
		errno = EINVAL;
		return -1;
	}

	if (button_find(*head, button, mod)) {
		errno = EEXIST;
		return -1;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return -1;
	}

	argv = args_clone(argv);
	if (argv == NULL) {
		return -1;
	}

	new->mod  = mod;
	new->argv = argv;

	new->next = *head;
	*head = new;

	return 0;
}

int
button_bind(int button, int mod, char *argv[])
{
	if (button < 0 || button > sizeof buttons / sizeof *buttons) {
		errno = EINVAL;
		return -1;
	}

	return button_add(&buttons[button], button, mod, argv);
}

int
button_dispatch(int button, int mod)
{
	struct button *p;

	if (button < 0 || button > sizeof buttons / sizeof *buttons) {
		errno = EINVAL;
		return -1;
	}

	p = button_find(buttons[button], button, mod);
	if (p == NULL) {
		errno = ENOENT;
		return -1;
	}

	return cmd_dispatch(p->argv);
}

