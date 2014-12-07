#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdarg.h>
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

void
event_issue(enum event_type type, const char *fmt, ...)
{
	struct sub *p;
	size_t n;
	va_list ap;
	char buf[64];

	assert(fmt != NULL);

	va_start(ap, fmt);

	n = vsnprintf(buf, sizeof buf, fmt, ap);
	if (n >= sizeof buf) {
		errno = ENOMEM;
		perror(buf);
		exit(EXIT_FAILURE);
	}

	va_end(ap);

	for (p = subs; p != NULL; p = p->next) {
		struct iovec iov[2];

		iov[0].iov_base = buf;  iov[0].iov_len = n;
		iov[1].iov_base = "\n"; iov[1].iov_len = 1;

		if (~p->mask & type) {
			continue;
		}

		if (-1 == writev(p->s, iov, sizeof iov / sizeof *iov)) {
			perror("send"); /* TODO: print peer address */
			/* TODO: cull peer */
			continue;
		}
	}
}

