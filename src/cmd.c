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
#include "button.h"
#include "layout.h"
#include "frame.h"
#include "key.h"

static int
delta(const char *s)
{
	size_t i;
	long l;

	struct {
		const char *name;
		int n;
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
			return a[i].n;
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

static int
cmd_spawn(char *argv[])
{
	int r;

	r = fork();
	if (r == -1) {
		return -1;
	}

	if (r != 0) {
		return 0;
	}

	/* TODO: getopt -d to detach child */

	if (-1 == execvp(argv[0], argv)) {
		perror(argv[0]);
	}

	exit(1);
}

static int
cmd_keybind(char *argv[])
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
cmd_mousebind(char *argv[])
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
cmd_prepend(char *argv[])
{
	struct frame *new;

	new = frame_split(current_frame, ORDER_PREV);
	if (new == NULL) {
		return -1;
	}

	current_frame = new;

	return 0;
}

static int
cmd_append(char *argv[])
{
	struct frame *new;

	new = frame_split(current_frame, ORDER_NEXT);
	if (new == NULL) {
		return -1;
	}

	current_frame = new;

	return 0;
}

static int
cmd_sibling(char *argv[])
{
	struct frame *new;
	int d;

	/* TODO: -f -w for frame/window siblings */

	d = delta(argv[0]);
	if (d == 0) {
		return -1;
	}

	new = frame_sibling(current_frame, d);
	if (new == NULL) {
		return -1;
	}

	current_frame = new;

	return 0;
}

static int
cmd_layout(char *argv[])
{
	enum layout *curr, l;
	int d;

	if (0 == strcmp(argv[0], "-f")) {
		curr = &current_frame->frame_layout;
	} else if (0 == strcmp(argv[0], "-w")) {
		curr = &current_frame->window_layout;
	} else {
		errno = EINVAL;
		return -1;
	}

	l = layout_lookup(argv[1]);
	if (l == -1) {
		d = delta(argv[1]);
		if (d == 0) {
			return -1;
		}

		l = layout_cycle(*curr, d);
	}

	*curr = l;

	/* TODO: redraw frame */

	return 0;
}

int
cmd_dispatch(char *argv[])
{
	size_t i;

	struct {
		const char *cmd;
		int (*f)(char *[]);
	} a[] = {
		{ "keybind",   cmd_keybind   },
		{ "mousebind", cmd_mousebind },
		{ "spawn",     cmd_spawn     },
		{ "prepend",   cmd_prepend   },
		{ "append",    cmd_append    },
		{ "sibling",   cmd_sibling   },
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

