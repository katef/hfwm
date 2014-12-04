#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h> /* XXX */

#include <X11/X.h>

#include "key.h"

static struct key *keys;

static const struct {
	const char *name;
	int button;
	int mask;
} buttons[] = {
	{ "Button1", Button1, Button1Mask },
	{ "Button2", Button2, Button2Mask },
	{ "Button3", Button3, Button3Mask },
	{ "Button4", Button4, Button4Mask },
	{ "Button5", Button5, Button5Mask }
};

int
button_lookup(const char *s)
{
	size_t i;

	assert(s != NULL);

	for (i = 0; i < sizeof buttons / sizeof *buttons; i++) {
		if (0 == strcmp(buttons[i].name, s)) {
			return buttons[i].button;
		}
	}

	return 0;
}

int
button_mask(int n)
{
	size_t i;

	for (i = 0; i < sizeof buttons / sizeof *buttons; i++) {
		if (buttons[i].button == n) {
			return buttons[i].mask;
		}
	}

	return 0;
}

int
mod_lookup(const char *s)
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
		{ "Mod5",    Mod5Mask    }
	};

	assert(s != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].s)) {
			return a[i].mod;
		}
	}

	for (i = 0; i < sizeof buttons / sizeof *buttons; i++) {
		if (0 == strcmp(s, buttons[i].name)) {
			return buttons[i].mask;
		}
	}

	return 0;
}

int
mod_prefix(const char *s, const char **e)
{
	size_t n;
	int mod;

	assert(s != NULL);

	mod = 0;

	for ( ; ; s += n + 1) {
		char buf[16];
		int m;

		n = strcspn(s, "-");
		if (n == 0 || n > sizeof buf - 1) {
			break;
		}

		memcpy(buf, s, n);
		buf[n] = '\0';

		m = mod_lookup(buf);
		if (m == 0) {
			break;
		}

		mod |= m;
	}

	if (e != NULL) {
		*e = s;
	}

	return mod;
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

