/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef CHAIN_H
#define CHAIN_H

struct chain;

struct chain *
chain_append(struct chain **head, char *const *argv);

int
chain_dispatch(const struct chain *chain);

void
chain_free(struct chain *chain);

#endif

