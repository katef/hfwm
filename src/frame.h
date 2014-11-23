#ifndef FRAME_H
#define FRAME_H

struct frame;

extern struct frame *current_frame;

struct frame *
frame_create(void);

struct frame *
frame_prepend(struct frame **head);

struct frame *
frame_append(struct frame **head);

#endif

