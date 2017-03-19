/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef CURRENT_H
#define CURRENT_H

struct frame;
struct client;

extern struct frame *current_frame;

void
set_current_frame(struct frame *p);

void
set_current_client(struct frame *p, struct client *q);

#endif

