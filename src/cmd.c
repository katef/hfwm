#include <unistd.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "cmd.h"
#include "main.h"

int
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

	/* TODO: should probably detach child */

	if (-1 == execvp(argv[0], argv)) {
		perror(argv[0]);
	}

	exit(1);
}

int
cmd_keybind(char *argv[])
{
	KeySym ks;
	int kc;

	/* TODO: parse for mod */

	ks = XStringToKeysym(argv[0]);
	if (ks == NoSymbol) {
		return -1;
	}

	kc = XKeysymToKeycode(display, ks);
	if (kc == 0) {
		return -1;
	}

	if (!XGrabKey(display, kc, MOD, root, True, GrabModeAsync, GrabModeAsync)) {
		return -1;
	}

	/* TODO: register command */

	return 0;
}

int
cmd_mousebind(char *argv[])
{
	int button;
	char *e;

	/* TODO: parse for mod */

	button = strtol(argv[0], &e, 10);
	if (button < 0 || button == ULONG_MAX || button > INT_MAX) {
		errno = ERANGE;
		return -1;
	}

	if (*e != '\0') {
		errno = EINVAL;
		return -1;
	}

	if (!XGrabButton(display, button, MOD, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None)) {
		return -1;
	}

	/* TODO: register command */

	return 0;
}

