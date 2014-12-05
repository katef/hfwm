#define _XOPEN_SOURCE 500

#include <unistd.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "geom.h"
#include "win.h"

char *hostname; /* for WM_CLIENT_MACHINE */

int screen;
Display *display;
Window root;

Window
win_create(const struct geom *geom, const char *name, const char *class,
	unsigned int bw, unsigned int spacing)
{
	Window win;
	unsigned long valuemask;
	XSetWindowAttributes attrs;
	XTextProperty xtp_name, xtp_client;
	XWMHints wm_hints;
	XSizeHints size_hints;
	XClassHint class_hints;
	char *argv[] = { NULL };
	struct geom in;
	XVisualInfo vinfo;

const char *bg = NULL;

	assert(geom != NULL);
	assert(name != NULL);
	assert(class != NULL);

	valuemask = CWEventMask | CWBackPixel;

	attrs.event_mask = EnterWindowMask;

/* XXX: want to do this in a seperate function i think. or maybe as external event handling */

	if (0 == XStringListToTextProperty((char **) &name, 1, &xtp_name)) {
		perror("XStringListToTextProperty");
		exit(EXIT_FAILURE);
	}

	if (-1 == geom_inner(&in, geom, bw, spacing)) {
		return (Window) 0x0;
	}

	if (bg != NULL) {
		Colormap cm;
		XColor col;

		cm = DefaultColormap(display, 0);

		XParseColor(display, cm, bg, &col);
		XAllocColor(display, cm, &col);

		attrs.background_pixel = col.pixel;

		vinfo.depth  = CopyFromParent;
		vinfo.visual = CopyFromParent;

		valuemask |= CWBackPixel;
	} else {
		Colormap cm;

		XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo);

		cm = XCreateColormap(display, root, vinfo.visual, AllocNone);

		/*
		 * XXX: why is this alpha pixel mutually exclusive with a colour value?
		 * Can't I have RGBA instead of 100% transparency?
		 * Apparently I can use GLX for this.
		 */

		attrs.colormap         = cm;
		attrs.background_pixel = 0;

		valuemask |= CWColormap | CWBackPixel;
	}

	win = XCreateWindow(display, root,
		in.x, in.y, in.w, in.h,
		bw,
		vinfo.depth,
		InputOutput,
		vinfo.visual,
		valuemask, &attrs);

	class_hints.res_name  = "hfwm"; /* note not WM_NAME */
	class_hints.res_class = (char *) class;

	wm_hints.flags = InputHint | StateHint;
	wm_hints.initial_state = NormalState;

	size_hints.flags = 0;

	XSetWMProperties(display, win,
		&xtp_name, &xtp_name,
		argv, sizeof argv / sizeof *argv - 1,
		&size_hints, &wm_hints, &class_hints);

	/*
	 * WM_CLIENT_MACHINE is supposed to be the hostname of the client's machine
	 * from the perspective of the X11 server. There's no way we could possibly
	 * know that, if there even is one. I'm setting the hostname instead,
	 * because at least that might be a useful reminder for a human.
	 */
	if (hostname != NULL) {
		if (0 == XStringListToTextProperty(&hostname, 1, &xtp_client)) {
			perror("XStringListToTextProperty");
			exit(EXIT_FAILURE);
		}

		XSetWMClientMachine(display, win, &xtp_client);
	}

	XMapWindow(display, win);

	return win;
}

void
win_destroy(Window win)
{
	XDestroyWindow(display, win);
}

int
win_resize(Window win, const struct geom *geom,
	unsigned int bw, unsigned int spacing)
{
	struct geom in;

	assert(geom != NULL);

	if (-1 == geom_inner(&in, geom, bw, spacing)) {
		return -1;
	}

	XMoveResizeWindow(display, win,
		in.x, in.y, in.w, in.h);

	return 0;
}

void
win_border(Window win, const char *colour)
{
	Colormap cm;
	XColor col;

	assert(colour != NULL);

	cm = DefaultColormap(display, 0);

	XParseColor(display, cm, colour, &col);
	XAllocColor(display, cm, &col);

	XSetWindowBorder(display, win, col.pixel);
}

int
win_geom(Window win, struct geom *geom)
{
	int x, y;
	unsigned int w, h;
	Window rr;
	unsigned int bw;
	unsigned int depth;

	assert(geom != NULL);

	XGetGeometry(display, win, &rr, &x, &y, &w, &h, &bw, &depth);

	if (x < 0 || y < 0) {
		errno = EINVAL;
		return -1;
	}

	if (w == 0 || h == 0) {
		errno = EINVAL;
		return -1;
	}

	geom->x = x;
	geom->y = y;
	geom->w = w;
	geom->h = h;

	return 0;
}

