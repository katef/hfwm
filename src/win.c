#define _XOPEN_SOURCE 500

#include <unistd.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "main.h"
#include "geom.h"
#include "win.h"

Window
win_create(const struct geom *geom, const char *name, const char *class)
{
	Window win;
	unsigned long valuemask;
	XSetWindowAttributes attrs;
	XTextProperty xtp_name, xtp_client;
	XWMHints wm_hints;
	XSizeHints size_hints;
	XClassHint class_hints;
	char *argv[] = { NULL };
	char *client = hostname;
	struct geom in;

	assert(geom != NULL);
	assert(name != NULL);
	assert(class != NULL);

	valuemask = CWEventMask;

	attrs.event_mask = EnterWindowMask;

	if (0 == XStringListToTextProperty((char **) &name, 1, &xtp_name)) {
		perror("XStringListToTextProperty");
		exit(EXIT_FAILURE);
	}

	if (0 == XStringListToTextProperty(&client, 1, &xtp_client)) {
		perror("XStringListToTextProperty");
		exit(EXIT_FAILURE);
	}

	if (-1 == geom_inner(&in, geom, WIN_BORDER)) {
		return (Window) 0x0;
	}

	win = XCreateWindow(display, root,
		in.x, in.y, in.w, in.h,
		WIN_BORDER,
		CopyFromParent, /* XXX: why not InputOutput? */
		CopyFromParent,
		CopyFromParent,
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
	XSetWMClientMachine(display, win, &xtp_client);

	XMapWindow(display, win);

	return win;
}

int
win_resize(Window win, const struct geom *geom)
{
	struct geom in;

	assert(geom != NULL);

	if (-1 == geom_inner(&in, geom, WIN_BORDER)) {
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

void
win_cat(struct window **head, struct window **tail)
{
	struct window **p;

	assert(head != NULL);

	for (p = head; *p != NULL; p = &(*p)->next)
		;

	*p    = *tail;
	*tail = NULL;
}

