#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

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

