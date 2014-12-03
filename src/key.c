#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/X.h>

#include "key.h"

static struct key *keys;

int
button_mask(int n)
{
	static const int mask[] = {
		Button1Mask,
		Button2Mask,
		Button3Mask,
		Button4Mask,
		Button5Mask
	};

	if (n < 0 || n > sizeof mask / sizeof *mask) {
		errno = ERANGE;
		return 0;
	}

	return mask[n];
}

int
key_mod(const char *s)
{
	size_t i;

	struct {
		const char *s;
		int mod;
	} a[] = {
		{ "Shift",   ShiftMask   },
		{ "Lock",    LockMask    },
		{ "Ctrl",    ControlMask },

		{ "Mod1",    Mod1Mask    },
		{ "Mod2",    Mod2Mask    },
		{ "Mod3",    Mod3Mask    },
		{ "Mod4",    Mod4Mask    },
		{ "Mod5",    Mod5Mask    },

		{ "Button1", Button1Mask },
		{ "Button2", Button2Mask },
		{ "Button3", Button3Mask },
		{ "Button4", Button4Mask },
		{ "Button5", Button5Mask }
	};

	assert(s != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].s)) {
			return a[i].mod;
		}
	}

	return 0;
}

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

