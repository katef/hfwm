#define _XOPEN_SOURCE 600

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

#include "args.h"
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
#include "client.h"
#include "tile.h"
#include "chain.h"
#include "event.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifndef TMPDIR
#define TMPDIR "/tmp"
#endif

#define HFWM_VERSION "0.1"

#ifndef IPC_NAME_
#define IPC_NAME ".hfwm"
#endif

#ifndef IPC_ENV
#define IPC_ENV "HFWM_IPC"
#endif

static char ipc_path[PATH_MAX];

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
			{
				struct key *p;

				p = key_find(e.xkey.keycode, e.xkey.state);
				if (p == NULL) {
					continue;
				}

				if (-1 == chain_dispatch(p->chain)) {
					perror("chain_dispatch"); /* XXX: better error message */
					continue;
				}
			}
			break;

		case ButtonPress:
			break;

		case ButtonRelease:
			{
				struct key *p;
				int mask;

				mask = button_mask(e.xbutton.button);
				if (mask == 0) {
					fprintf(stderr, "unrecognised button %d\n", e.xbutton.button);
					continue;
				}

				p = key_find(AnyKey, mask | e.xbutton.state);
				if (p == NULL) {
					continue;
				}

				if (-1 == chain_dispatch(p->chain)) {
					perror("chain_dispatch"); /* XXX: better error message */
					continue;
				}
			}
			break;

		case CreateNotify:
			XSetWindowBorderWidth(display, e.xcreatewindow.window, TILE_BORDER);
			XSelectInput(display, e.xcreatewindow.window, EnterWindowMask | LeaveWindowMask);
			break;

		case DestroyNotify:
			/* TODO: also need to remove windows on DestroyNotify?
			 * are windows always unmapped first? */
			break;

		case EnterNotify:
		case LeaveNotify:
			event_issue(EVENT_CROSSING, "%s %s %p",
				e.type == EnterNotify ? "enter" : "leave",
				win_type(e.xcrossing.window),
				(void *) e.xcrossing.window);
			break;

		case MapRequest:
			/* TODO: this is where to grab e.g tabs window and reparent per frame (or root) */
			XMapWindow(display, e.xmaprequest.window);
			break;

		case MapNotify:
			if (current_frame->type != FRAME_LEAF) {
				/* TODO: find a leaf to put it in, instead. maybe have a current_leaf variable? */
				event_issue(EVENT_EXTANCE, "map unmanaged %p",
					(void *) e.xmap.window);
				continue;
			}

			{
				const struct frame *top;
				const struct frame *r;

				top = frame_top();
				assert(top != NULL);

				r = frame_find_win(top, e.xmap.window);
				if (r != NULL) {
					/* our frame's window */
					event_issue(EVENT_EXTANCE, "map %s %p",
						frame_type(r),
						(void *) e.xmap.window);
					continue;
				}
			}

			{
				struct client *new;

				new = client_add(&current_frame->u.clients, e.xmap.window);
				if (new == NULL) {
					perror("client_add");
					continue;
				}

				/* XXX: focusid on IPC map event instead */
				set_current_client(current_frame, new);
			}

			tile_clients(current_frame->u.clients, current_frame->layout,
				&current_frame->geom, current_frame->current_client);

			event_issue(EVENT_EXTANCE, "map client %p",
				(void *) e.xmap.window);

			break;

		case UnmapNotify:
			{
				const struct frame *top;
				struct frame *r;

				top = frame_top();
				assert(top != NULL);

				r = frame_find_win(top, e.xmap.window);
				/* TODO: if it's a frame... */
				if (r != NULL) {
					event_issue(EVENT_EXTANCE, "unmap %s %p",
						frame_type(r),
						(void *) e.xmap.window);
					continue;
				}

				r = frame_find_client(top, e.xmap.window);
				if (r == NULL) {
					event_issue(EVENT_EXTANCE, "unmap unmanaged %p",
						(void *) e.xmap.window);
					continue;
				}

				assert(r->type == FRAME_LEAF);
				set_current_client(r, NULL);

				client_remove(&r->u.clients, e.xmap.window);

				tile_clients(r->u.clients, r->layout, &r->geom,
					r->current_client);

				event_issue(EVENT_EXTANCE, "unmap client %p",
					(void *) e.xmap.window);
			}
			break;

		case ConfigureRequest:
			/* TODO */
			break;

		case ConfigureNotify:
			/* TODO */
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

static void
usage(FILE *f)
{
	assert(f != NULL);

	fprintf(f, "usage: hfwm [-f <startup>] [-m <hostname>] {-s <ipc name>}\n");
	fprintf(f, "       hfwm {-h | -v}\n");
}

static void
cleanup(void)
{
	if (-1 == unlink(ipc_path)) {
		perror(ipc_path);
	}
}

int
main(int argc, char *argv[])
{
	int x11, ipc;
	struct geom g;
	char *startup;
	const char *tmp;
	const char *ipc_dir;
	const char *ipc_name;

	tmp = getenv("TMPDIR");
	if (tmp == NULL) {
		tmp = TMPDIR;
	}

	/* defaults */
	startup  = NULL;
	hostname = NULL;
	ipc_dir  = tmp;
	ipc_name = IPC_NAME;

	{
		int c;

		while (c = getopt(argc, argv, "hv" "f:m:s:"), c != -1) {
			switch (c) {
			case 'f': startup  = optarg; break;
			case 'm': hostname = optarg; break;
			case 's': ipc_name = optarg; break;

			case 'h':
				usage(stdout);
				return 0;

			case 'v':
				printf("hfwm %s", HFWM_VERSION);
				return 0;

			case '?':
			default:
				usage(stderr);
				return 1;
			}
		}

		argc -= optind;
		argv += optind;

		if (argc > 0) {
			usage(stderr);
			return 1;
		}
	}

	{
		size_t n;

		n = snprintf(ipc_path, sizeof ipc_path, "%s/%s-XXXXXX", ipc_dir, ipc_name);
		if (n >= sizeof ipc_path) {
			perror(ipc_path);
			return 1;
		}

		if (-1 == mkstemp(ipc_path)) {
			perror(ipc_path);
			return 1;
		}
	}

	display = XOpenDisplay(NULL);
	screen  = DefaultScreen(display);
	root    = DefaultRootWindow(display); /* TODO: RootWindow() instead */

	x11 = ConnectionNumber(display);
	ipc = ipc_listen(ipc_path);

	(void) atexit(cleanup);

	XSelectInput(display, root,
		KeyPressMask
		| ButtonPressMask
		| EnterWindowMask | LeaveWindowMask
		| SubstructureRedirectMask | SubstructureNotifyMask);

	if (-1 == win_geom(root, &g)) {
		perror("win_geom");
		return 1;
	}

	if (-1 == geom_inset(&g, &g, 0, FRAME_MARGIN - FRAME_SPACING)) {
		perror("geom_inset");
		return 1;
	}

	current_frame = frame_create_leaf(&g, NULL);
	if (current_frame == NULL) {
		perror("frame_create_leaf");
		return 1;
	}

	event_issue(EVENT_DIOPTRE, "focus leaf %p",
		(void *) current_frame->win);

	if (startup != NULL) {
		char *argv[2];

		argv[0] = startup;
		argv[1] = NULL;

		if (-1 == setenv(IPC_ENV, ipc_path, 1)) {
			perror("setenv");
			return -1;
		}

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

