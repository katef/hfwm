#ifndef KEY_H
#define KEY_H

#ifndef MOD
#define MOD Mod4Mask
#endif

struct chain;

struct key {
	unsigned int keycode;
	int mod;
	struct chain *chain;
	struct key *next;
};

int
button_lookup(const char *s);

int
button_mask(int n);

int
mod_lookup(const char *s);

struct key *
key_find(unsigned int keycode, int mod);

struct key *
key_provision(unsigned int keycode, int mod);

#endif

