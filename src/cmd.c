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
#include "frame.h"
#include "button.h"
#include "key.h"

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
cmd_append(char *argv[])
{
	if (!frame_append(&current_frame)) {
		return -1;
	}

	return 0;
}

static int
cmd_prepend(char *argv[])
{
	if (!frame_prepend(&current_frame)) {
		return -1;
	}

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
		{ "append",    cmd_append    },
		{ "prepend",   cmd_prepend   }
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

