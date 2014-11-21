#ifndef MAIN_H
#define MAIN_H

#include <X11/Xlib.h>

Display *display;
Window root;

#ifndef MOD
#define MOD Mod4Mask
#endif

#endif

