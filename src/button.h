#ifndef BUTTON_H
#define BUTTON_H

#ifndef BUTTON_MAX
#define BUTTON_MAX 16 /* XXX: from X? */
#endif

struct chain;

struct button {
	int mod;
	struct chain *chain;
	struct button *next;
};

struct button *
button_find(int button, int mod);

struct button *
button_provision(int button, int mod);

#endif

