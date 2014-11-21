#ifndef KEY_H
#define KEY_H

int
key_bind(unsigned int keycode, int mod, char *argv[]);

int
key_dispatch(unsigned int keycode, int mod);

#endif

