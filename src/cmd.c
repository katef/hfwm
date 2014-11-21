#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"

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

	if (-1 == execvp(argv[0], argv)) {
		perror(argv[0]);
	}

	exit(1);
}

