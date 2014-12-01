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
#include "win.h"
#include "tile.h"

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

	XGrabKey(display, kc, mod, root, True, GrabModeAsync, GrabModeAsync);

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

	XGrabButton(display, button, mod, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

	if (-1 == button_bind(button, mod, argv + 1)) {
		return -1;
	}

	return 0;
}

static int
cmd_split(char *const argv[])
{
	struct frame *new;
	enum layout layout;
	enum order order;

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

		new->u.children = frame_create_leaf(new, &new->geom, NULL);
		if (new->u.children == NULL) {
			/* TODO: combine geometry (merge, undoing the split) */
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

/* TODO: redraw everything below this node */

	return 0;
}

static int
cmd_merge(char *const argv[])
{
	struct frame *new;
	enum layout layout;
	enum order order;

	assert(current_frame != NULL);

	order = order_lookup(argv[0]);
	if (order == -1) {
		return -1;
	}

	if (current_frame->parent == NULL) {
		layout = LAYOUT_MAX;
	} else {
		layout = current_frame->parent->layout;
	}

	new = frame_merge(current_frame, layout, order);
	if (new == NULL) {
		return -1;
	}

	current_frame = new;

/* TODO: redraw everything below this node */

	return 0;
}

static int
cmd_focus(char *const argv[])
{
	struct frame *new;
	enum order order;
	enum rel rel;

	/* TODO: -f -w for frame/window siblings */

	/* TODO: setting for inactive colour */
	if (current_frame->type == FRAME_LEAF) {
		win_border(current_frame->win, "#222222");
	}

	rel = rel_lookup(argv[0]);
	if (rel == -1) {
		return -1;
	}

	order = order_lookup(argv[1]);
	if (order == -1) {
		return -1;
	}

	new = frame_focus(current_frame, rel, order);
	if (new == NULL) {
		return -1;
	}

	current_frame = new;

	/* TODO: setting for active colour */
	if (current_frame->type == FRAME_LEAF) {
		win_border(current_frame->win, "#EE2222");
	}

	return 0;
}

static int
cmd_layout(char *const argv[])
{
	enum layout layout;
	enum order order;

	layout = layout_lookup(argv[0]);
	if (layout == -1) {
		int delta;

		order = order_lookup(argv[0]);
		if (order == -1) {
			return -1;
		}

		switch (order) {
		case ORDER_NEXT: delta = +1; break;
		case ORDER_PREV: delta = -1; break;
		}

		layout = layout_cycle(current_frame->layout, delta);
	}

	current_frame->layout = layout;

	if (-1 == tile_resize(current_frame)) {
		perror("tile_resize");
		return -1;
	}

	return 0;
}

static int
cmd_redist(char *const argv[])
{
	enum layout layout;
	enum order order;
	unsigned n;

	order = order_lookup(argv[0]);
	if (order == -1) {
		return -1;
	}

	n = atoi(argv[1]); /* TODO: error checking */

	if (current_frame->parent == NULL) {
		layout = LAYOUT_MAX;
	} else {
		layout = current_frame->parent->layout;
	}

	frame_redistribute(current_frame, layout, order, n);

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
		{ "merge",     cmd_merge     },
		{ "focus",     cmd_focus     },
		{ "layout",    cmd_layout    },
		{ "redist",    cmd_redist    }
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

