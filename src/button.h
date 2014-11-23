#ifndef BUTTON_H
#define BUTTON_H

#ifndef BUTTON_MAX
#define BUTTON_MAX 16 /* XXX: from X? */
#endif

int
button_bind(int button, int mod, char *const argv[]);

int
button_dispatch(int button, int mod);

#endif

