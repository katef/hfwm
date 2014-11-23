#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "spawn.h"

int
spawn(char *argv[])
{
	int r;

	assert(argv != NULL);
	assert(argv[0] != NULL);

	r = fork();
	if (r == -1) {
		return -1;
	}

	if (r != 0) {
/* TODO: handle SIGCHLD and wait for child */
		return 0;
	}

	/* TODO: flag to detach child */

	if (-1 == execvp(argv[0], argv)) {
		perror(argv[0]);
	}

	exit(1);
}

