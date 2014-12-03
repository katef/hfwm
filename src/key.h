#ifndef KEY_H
#define KEY_H

struct chain;

struct key {
	unsigned int keycode;
	int mod;
	struct chain *chain;
	struct key *next;
};

struct key *
key_find(unsigned int keycode, int mod);

struct key *
key_provision(unsigned int keycode, int mod);

#endif

