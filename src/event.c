#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "event.h"

struct sub {
	unsigned int mask;
	int s;
	struct sub *next;
};

struct sub *subs;

int
event_subscribe(const char *path, unsigned int mask)
{
	struct sockaddr_un sun;
	struct sub *new;
	socklen_t len;
	int s;

	assert(path != NULL);

	if (mask == 0) {
		return 0;
	}

	if (strlen(path) + 1 > sizeof sun.sun_path) {
		perror("IPC path too long");
		return -1;
	}

	s = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (s == -1) {
		perror("socket");
		return -1;
	}

	sun.sun_family = AF_LOCAL;
	strcpy(sun.sun_path, path);

	len = sizeof sun.sun_family + strlen(sun.sun_path);
	if (-1 == connect(s, (struct sockaddr *) &sun, len)) {
		perror(path);
		goto error;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		goto error;
	}

	new->s    = s;
	new->mask = mask;

	new->next = subs;
	subs = new;

	return 0;

error:

	close(s);

	return -1;
}

int
event_announce(enum event_type type, ...)
{
	struct sub *p;
	char buf[64];

	switch (type) {
	case EVENT_SOMETHING:
		sprintf(buf, "something");
		break;

	default:
		errno = EINVAL;
		return -1;
	}

	for (p = subs; p != NULL; p = p->next) {
		if (~p->mask & type) {
			continue;
		}

		if (-1 == send(p->s, buf, strlen(buf), 0)) {
			perror("send"); /* TODO: print peer address */
			/* TODO: cull peer */
			continue;
		}
	}

	return 0;
}

