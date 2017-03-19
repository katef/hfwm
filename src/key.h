/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

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

extern struct key *keys;

int
button_lookup(const char *s);

int
button_mask(int n);

int
mod_lookup(const char *s);

int
mod_prefix(const char *s, const char **e);

int
key_code(const char *s, unsigned int *kc, int *mod);

struct key *
key_find(unsigned int keycode, int mod);

struct key *
key_provision(unsigned int keycode, int mod);

#endif

