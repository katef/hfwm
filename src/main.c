#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "main.h"
#include "args.h"
#include "cmd.h"
#include "geom.h"
#include "order.h"
#include "layout.h"
#include "button.h"
#include "frame.h"
#include "spawn.h"
#include "key.h"
#include "win.h"
#include "client.h"
#include "tile.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

char hostname[HOST_NAME_MAX];
Display *display;
Window root;

#ifndef IPC_PATH
#define IPC_PATH "/tmp/hfwm.sock"
#endif

#ifndef HFWM_STARTUP
#define HFWM_STARTUP "startup.sh"
#endif

static int
ipc_listen(const char *path)
{
	struct sockaddr_un sun;
	socklen_t len;
	int s;

	assert(path != NULL);

	if (strlen(path) + 1 > sizeof sun.sun_path) {
		perror("IPC path too long");
		exit(1);
	}

	s = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (s == -1) {
		perror("socket");
		return 1;
	}

	sun.sun_family = AF_LOCAL;
	strcpy(sun.sun_path, path);

	if (-1 == unlink(sun.sun_path) && errno != ENOENT) {
		perror(sun.sun_path);
		return 1;
	}

	len = sizeof sun.sun_family + strlen(sun.sun_path);
	if (-1 == bind(s, (struct sockaddr *) &sun, len)) {
		perror(sun.sun_path);
		return 1;
	}

	return s;
}

static void
event_x11(void)
{
	while (XPending(display)) {
		XEvent e;

		XNextEvent(display, &e);

		switch (e.type) {
		case KeyPress:
			if (-1 == key_dispatch(e.xkey.keycode, e.xkey.state)) {
				perror("key_dispatch");
			}
			break;

		case ButtonPress:
		case ButtonRelease:
			if (-1 == button_dispatch(e.xbutton.button, e.xbutton.state)) {
				perror("button_dispatch");
			}
			break;

		case CreateNotify:
			if (current_frame->type != FRAME_LEAF) {
				continue;
			}

			{
				const struct frame *top;

				top = frame_top();
				assert(top != NULL);

				if (frame_find_win(top, e.xcreatewindow.window)) {
					/* our frame's window; disregard */
					continue;
				}
			}

			if (!client_add(&current_frame->u.clients, e.xcreatewindow.window)) {
				perror("client_add");
				continue;
			}

			XSetWindowBorderWidth(display, e.xcreatewindow.window, TILE_BORDER);

			/* TODO: would reparent here */

			if (-1 == tile_resize(current_frame)) {
				perror("layout_resize");
				/* TODO */
			}

			XRaiseWindow(display, e.xcreatewindow.window);

			XMapWindow(display, e.xcreatewindow.window);
			break;

		case DestroyNotify:
			{
				const struct frame *top;
				struct frame *r;

				top = frame_top();
				assert(top != NULL);

				r = frame_find_client(top, e.xcreatewindow.window);
				if (r == NULL) {
					continue;
				}

				client_remove(&r->u.clients, e.xcreatewindow.window);
			}
			break;

		case EnterNotify:
			{
				const struct frame *top;
				struct frame *r;

				top = frame_top();
				assert(top != NULL);

				r = frame_find_win(top, e.xcrossing.window);
				if (r == NULL) {
					/* not a frame window */
					continue;
				}

				/* TODO: setting for colours */

				if (current_frame->type == FRAME_LEAF) {
					win_border(current_frame->win, "#2222FF");
				}

				current_frame = r;

				win_border(current_frame->win, "#FF2222");
			}
			break;

		default:
			fprintf(stderr, "unhandled event %d\n", e.type);
			continue;
		}
	}
}

static void
event_ipc(int s)
{
	char buf[1024], hack[1024]; /* XXX: BUFSZ? */
	char *argv[64]; /* XXX: argc size? */
	ssize_t r;

	assert(s != -1);

	r = recvfrom(s, buf, sizeof buf, 0, NULL, 0);
	if (-1 == r) {
		perror("recvfrom");
		exit(1);
	}

	if (r == sizeof buf) {
		fprintf(stderr, "IPC overflow: %s\n", buf);
		return;
	}

	buf[r] = '\0';

	/* XXX: args() segfaults writing back to buf. why? */
	if (-1 == args(buf, hack, argv, sizeof argv / sizeof *argv)) {
		perror(buf);
		exit(1);
	}

	if (-1 == cmd_dispatch(argv)) {
		perror(argv[0]);
	}
}

int
main(void)
{
	int x11, ipc;
	struct geom g;

	if (-1 == gethostname(hostname, sizeof hostname)) {
		perror("gethostname");
		return 1;
	}

	display = XOpenDisplay(NULL);
	root    = DefaultRootWindow(display); /* TODO: RootWindow() instead */

	x11 = ConnectionNumber(display);
	ipc = ipc_listen(IPC_PATH);

	XSelectInput(display, root,
		SubstructureRedirectMask | SubstructureNotifyMask);

	if (-1 == win_geom(root, &g)) {
		perror("win_geom");
		return 1;
	}

	if (-1 == geom_inner(&g, &g, 0, FRAME_MARGIN - FRAME_SPACING)) {
		perror("geom_inner");
		return 1;
	}

	current_frame = frame_create_leaf(NULL, &g, NULL);
	if (current_frame == NULL) {
		perror("frame_create");
		return 1;
	}

	{
		char *argv[] = { HFWM_STARTUP, NULL };

		if (-1 == spawn(argv)) {
			perror(argv[0]);
			return -1;
		}
	}

	for (;;) {
		fd_set fds;
		int max;
		int r;

		max = 0;
		FD_ZERO(&fds);
		FD_SET(x11, &fds); max = MAX(max, x11);
		FD_SET(ipc, &fds); max = MAX(max, ipc);

		XFlush(display);

		r = select(max + 1, &fds, 0, 0, NULL);
		if (r == -1) {
			perror("select");
			return 1;
		}

		if (r == 0) {
			continue;
		}

		if (FD_ISSET(x11, &fds)) {
			event_x11();
		}

		if (FD_ISSET(ipc, &fds)) {
			event_ipc(ipc);
		}
	}

	return 0;
}

