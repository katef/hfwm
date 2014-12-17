#include <unistd.h>

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "cmd.h"
#include "axis.h"
#include "geom.h"
#include "order.h"
#include "layout.h"
#include "current.h"
#include "frame.h"
#include "spawn.h"
#include "key.h"
#include "win.h"
#include "tile.h"
#include "args.h"
#include "chain.h"
#include "client.h"
#include "event.h"

static int
cmd_subscribe(char *const argv[])
{
	unsigned int mask;

	mask = ~0U; /* TODO: parse from argv[1] */

	if (-1 == event_subscribe(argv[0], mask)) {
		perror(argv[0]);
		return -1;
	}

	return 0;
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
cmd_bind(char *const argv[])
{
	unsigned int kc;
	struct key *p;
	char **args;
	int mod;

	if (-1 == key_code(argv[0], &kc, &mod)) {
		return -1;
	}

	p = key_provision(kc, mod);
	if (p == NULL) {
		goto error;
	}

	args = args_clone(argv + 1);
	if (args == NULL) {
		return -1;
	}

	if (!chain_append(&p->chain, args)) {
		goto error;
	}

	return 0;

error:

	free(args);

	return -1;
}

static int
cmd_unbind(char *const argv[])
{
	unsigned int kc;
	struct key *p;
	int mod;

	if (argv[0] == NULL) {
		for (p = keys; p != NULL; p = p->next) {
			chain_free(p->chain);
			p->chain = NULL;
		}

		return 0;
	}

	if (-1 == key_code(argv[0], &kc, &mod)) {
		return -1;
	}

	p = key_find(kc, mod);
	if (p == NULL) {
		return 0;
	}

	chain_free(p->chain);
	p->chain = NULL;

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
	if ((int) order == -1) {
		return -1;
	}

	switch (current_frame->type) {
	case FRAME_BRANCH:
		if (current_frame->parent == NULL) {
			layout = LAYOUT_MAX;
		} else {
			layout = current_frame->parent->layout;
		}
		break;

	case FRAME_LEAF:
		if (argv[1] == NULL) {
			layout = default_branch_layout;
		} else {
			layout = layout_lookup(argv[1]);
		}
		break;

	default:
		errno = EINVAL;
		return -1;
	}

	new = frame_branch(current_frame, layout, order);
	if (new == NULL) {
		return -1;
	}

	set_current_frame(new);

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
	if ((int) order == -1) {
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

	set_current_frame(new);

/* TODO: redraw everything below this node */

	return 0;
}

/* TODO: merge into cmd_focus after the dust settles */
static int
cmd_focusid(char *const argv[])
{
	Window win;

	/* TODO: move to win.c */
	{
		void *q;
		int r;

		r = sscanf(argv[0], "%p", &q);
		if (1 != r) {
			if (r >= 0) {
				errno = EINVAL;
			}
			return -1;
		}

		win = (Window) q;
	}

	if (0 == strcmp(argv[1], "client")) {
		struct client *p;

		if (current_frame->type != FRAME_LEAF) {
			errno = EINVAL;
			return -1;
		}

		p = client_find(current_frame->u.clients, win);
		if (p == NULL) {
			/* TODO: search all frames instead, and focus that frame first */
			errno = ENOENT;
			return -1;
		}

		set_current_client(current_frame, p);
	} else if (0 == strcmp(argv[1], "frame")) {
		const struct frame *top;
		struct frame *p;

		top = frame_top();
		assert(top != NULL);

		p = frame_find_win(top, win);
		if (p == NULL) {
			errno = ENOENT;
			return -1;
		}

		set_current_frame(p);
	} else {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
cmd_focus(char *const argv[])
{
	enum order order;

	order = order_lookup(argv[0]);
	if ((int) order == -1) {
		return -1;
	}

	if (0 == strcmp(argv[1], "client")) {
		struct client *new;

		if (current_frame->type != FRAME_LEAF) {
			return 0;
		}

		new = client_cycle(current_frame->u.clients, current_frame->current_client, order);

		set_current_client(current_frame, new);
	} else {
		struct frame *new;
		enum rel rel;

		rel = rel_lookup(argv[1]);
		if ((int) rel == -1) {
			return -1;
		}

		assert(current_frame != NULL);

		new = frame_focus(current_frame, rel, order);
		if (new == NULL) {
			return -1;
		}

		set_current_frame(new);
	}

	return 0;
}

static int
cmd_layout(char *const argv[])
{
	enum layout layout;
	enum order order;

	layout = layout_lookup(argv[0]);
	if ((int) layout == -1) {
		int delta;

		order = order_lookup(argv[0]);
		if ((int) order == -1) {
			return -1;
		}

		switch (order) {
		case ORDER_NEXT: delta = +1; break;
		case ORDER_PREV: delta = -1; break;
		}

		layout = layout_cycle(current_frame->layout, delta);
	}

	tile_clients(current_frame->u.clients, layout, &current_frame->geom,
		current_frame->current_client);

	current_frame->layout = layout;

	return 0;
}

static int
cmd_redist(char *const argv[])
{
	enum layout layout;
	enum order order;
	unsigned n;

	order = order_lookup(argv[0]);
	if ((int) order == -1) {
		return -1;
	}

	n = atoi(argv[1]); /* TODO: error checking */

	if (current_frame->parent == NULL) {
		layout = LAYOUT_MAX;
	} else {
		layout = current_frame->parent->layout;
	}

	if (-1 == frame_redistribute(current_frame, layout, order, n)) {
		return -1;
	}

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
		{ "subscribe", cmd_subscribe },
		{ "bind",      cmd_bind      },
		{ "unbind",    cmd_unbind    },
		{ "spawn",     cmd_spawn     },
		{ "split",     cmd_split     },
		{ "merge",     cmd_merge     },
		{ "focus",     cmd_focus     },
		{ "focusid",   cmd_focusid   },
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
/* TODO: bail out */
			}

			return 0;
		}
	}

	errno = ENOENT;
	return -1;
}

