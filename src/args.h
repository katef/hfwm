/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ARGS_H
#define ARGS_H

int
args(const char *src, char *dst, char *argv[], int count);

int
args_count(char *const argv[]);

char **
args_clone(char *const argv[]);

#endif

