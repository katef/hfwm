#ifndef FRAME_H
#define FRAME_H

struct frame;

extern struct frame *current_frame;

struct frame *
frame_create(void);

struct frame *
frame_split(struct frame *old, enum order order);

struct frame *
frame_sibling(struct frame *curr, int delta);

#endif

