#include <unistd.h>

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "cmd.h"
#include "main.h"
#include "geom.h"
#include "order.h"
#include "layout.h"
#include "button.h"
#include "frame.h"
#include "spawn.h"
#include "key.h"

static enum rel
rel_lookup(const char *s)
{
	size_t i;

	struct {
		const char *name;
		enum rel rel;
	} a[] = {
		{ "-s", REL_SIBLING },
		{ "-l", REL_LINEAGE }
	};

	if (s == NULL) {
		return REL_SIBLING;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].name)) {
			return a[i].rel;
		}
	}

	return -1;
}

static int
delta_lookup(const char *s)
{
	size_t i;
	long l;

	struct {
		const char *name;
		int delta;
	} a[] = {
		{ "first", INT_MIN },
		{ "last",  INT_MAX },
		{ "next",       +1 },
		{ "prev",       -1 }
	};

	if (s == NULL) {
		return +1;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].name)) {
			return a[i].delta;
		}
	}

	{
		char *e;

		errno = 0;

		l = strtol(s, &e, 0);
		if (*s == '\0' || *e != '\0') {
			errno = EINVAL;
			return 0;
		}

		if ((l == LONG_MIN || l == LONG_MAX) && errno != 0) {
			return 0;
		}

		if (l <= INT_MIN || l >= INT_MAX) {
			errno = ERANGE;
			return 0;
		}

		if (l == 0) {
			errno = EINVAL;
			return 0;
		}
	}

	return (int) l;
}

/* TODO: getopt with -n/-p instead */
static enum order
order_lookup(const char *s)
{
	size_t i;

	struct {
		const char *name;
		enum order order;
	} a[] = {
		{ "prev", ORDER_PREV },
		{ "next", ORDER_NEXT }
	};

	if (s == NULL) {
		return ORDER_NEXT;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].name)) {
			return a[i].order;
		}
	}

	errno = EINVAL;
	return -1;
}

static int
cmd_spawn(char *const argv[])
{
	/* TODO: getopt -d to detach child */

	if (-1 == spawn(argv)) {
		perror(argv[0]);
		return -1;
	}

	return 0;
}

static int
cmd_keybind(char *const argv[])
{
	int mod;
	KeySym ks;
	unsigned int kc;

	/* TODO: parse for mod */
	mod = MOD;

	ks = XStringToKeysym(argv[0]);
	if (ks == NoSymbol) {
		return -1;
	}

	kc = XKeysymToKeycode(display, ks);
	if (kc == 0) {
		return -1;
	}

	if (!XGrabKey(display, kc, mod, root, True, GrabModeAsync, GrabModeAsync)) {
		return -1;
	}

	if (-1 == key_bind(kc, mod, argv + 1)) {
		return -1;
	}

	return 0;
}

static int
cmd_mousebind(char *const argv[])
{
	int mod;
	int button;
	char *e;

	/* TODO: parse for mod */
	mod = MOD;

	button = strtol(argv[0], &e, 10);
	if (button < 0 || button == ULONG_MAX || button > BUTTON_MAX) {
		errno = ERANGE;
		return -1;
	}

	if (*e != '\0') {
		errno = EINVAL;
		return -1;
	}

	if (!XGrabButton(display, button, mod, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None)) {
		return -1;
	}

	if (-1 == button_bind(button, mod, argv + 1)) {
		return -1;
	}

	return 0;
}

static int
cmd_split(char *const argv[])
{
	struct frame *new;
	enum order order;
	enum layout layout;

	assert(current_frame != NULL);

	order = order_lookup(argv[0]);
	if (order == -1) {
		return -1;
	}

	switch (current_frame->type) {
	case FRAME_BRANCH:
		if (current_frame->parent == NULL) {
			layout = LAYOUT_MAX;
		} else {
			layout = current_frame->parent->layout;
		}

		new = frame_split(current_frame, layout, order);
		if (new == NULL) {
			return -1;
		}

		break;

	case FRAME_LEAF:
		if (argv[1] == NULL) {
			layout = default_branch_layout;
		} else {
			layout = layout_lookup(argv[1]);
		}

		new = frame_branch_leaf(current_frame, layout, order, NULL);
		if (new == NULL) {
			return -1;
		}

		break;
	}

	current_frame = new;

	return 0;
}

static int
cmd_focus(char *const argv[])
{
	struct frame *new;
	enum rel rel;
	int delta;

	/* TODO: -f -w for frame/window siblings */

	rel = rel_lookup(argv[0]);
	if (rel == -1) {
		return -1;
	}

	delta = delta_lookup(argv[1]);
	if (delta == 0) {
		return -1;
	}

	new = frame_focus(current_frame, rel, delta);
	if (new == NULL) {
		return -1;
	}

fprintf(stderr, "focus, current = %p\n", (void *) current_frame);
	current_frame = new;

	return 0;
}

static int
cmd_layout(char *const argv[])
{
	enum layout layout;
	int delta;

	layout = layout_lookup(argv[0]);
	if (layout == -1) {
		delta = delta_lookup(argv[1]);
		if (delta == 0) {
			return -1;
		}

		layout = layout_cycle(current_frame->layout, delta);
	}

	current_frame->layout = layout;

	/* TODO: redraw frame */

	return 0;
}

int
cmd_dispatch(char *const argv[])
{
	size_t i;

	struct {
		const char *cmd;
		int (*f)(char *const []);
	} a[] = {
		{ "keybind",   cmd_keybind   },
		{ "mousebind", cmd_mousebind },
		{ "spawn",     cmd_spawn     },
		{ "split",     cmd_split     },
		{ "focus",     cmd_focus     },
		{ "layout",    cmd_layout    }
	};

	assert(argv != NULL);

	if (argv[0] == NULL) {
		errno = EINVAL;
		return -1;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].cmd, argv[0])) {
			if (-1 == a[i].f(argv + 1)) {
				perror(argv[0]);
			}

			return 0;
		}
	}

	errno = ENOENT;
	return -1;
}

