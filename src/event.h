#ifndef EVENT_H
#define EVENT_H

enum event_type {
	EVENT_EXTANCE  = 1 << 0,
	EVENT_DIOPTRE  = 1 << 1,
	EVENT_CROSSING = 1 << 2
};

int
event_subscribe(const char *path, unsigned int mask);

void
event_issue(enum event_type type, const char *fmt, ...);

#endif

