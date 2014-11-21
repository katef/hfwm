#include <stdio.h>

#include "cmd.h"

int
cmd_spawn(char *argv[])
{
	fprintf(stderr, "cmd spawn %s\n", argv[0]);

	return 0;
}

