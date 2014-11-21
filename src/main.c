#include <sys/types.h>
#include <sys/select.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

Display *display;
Window root;

unsigned int mod = Mod4Mask;

void
dispatch_mouse(const XEvent *e)
{
	assert(e->type == ButtonPress || e->type == ButtonRelease || e->type == MotionNotify);
}

void
dispatch_key(const XEvent *e)
{
	assert(e->type == KeyPress);
}

static void
event_x11(void)
{
	while (XPending(display)) {
		XEvent e;
		int r;

		r = XNextEvent(display, &e);
		if (r != 0 && errno == EAGAIN) {
			break;

		}

		if (r != 0) {
			perror("XNextEvent");
			exit(1);
		}

		switch (e.type) {
		case KeyPress:      dispatch_key(&e);   break;
		case ButtonPress:   dispatch_mouse(&e); break;
		case ButtonRelease: dispatch_mouse(&e); break;
		case MotionNotify:  dispatch_mouse(&e); break;

		default:
			fprintf(stderr, "unhandled event %d\n", e.type);
			continue;
		}
	}
}

int
main(void)
{
	int x11;

	display = XOpenDisplay(NULL);
	if (display == NULL) {
		perror("XOpenDisplay");
		return 1;
	}

	x11 = ConnectionNumber(display);
	/* ipc = TODO: text protocol socket */

	root = DefaultRootWindow(display); /* TODO: RootWindow() instead */

	XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("a")), mod, root, True, GrabModeAsync, GrabModeAsync);

	XGrabButton(display, 1, mod, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(display, 3, mod, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

	if (!XFlush(display)) {
		perror("XFlush");
		return 1;
	}

	for (;;) {
		fd_set fds;
		int max;
		int r;

		max = 0;
		FD_ZERO(&fds);
		FD_SET(x11, &fds); max = MAX(max, x11);

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
	}

	return 0;
}

