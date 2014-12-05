#ifndef EVENT_H
#define EVENT_H

enum event_type {
	EVENT_SOMETHING = 0x01
};

int
event_subscribe(const char *path, unsigned int mask);

int
event_announce(enum event_type type, ...);

#endif

