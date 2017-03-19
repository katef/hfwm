/*
 * Copyright 2014-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ORDER_H
#define ORDER_H

enum order {
	ORDER_PREV,
	ORDER_NEXT
};

enum order
order_lookup(const char *s);

#endif

