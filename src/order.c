#include <assert.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "order.h"

enum order
order_lookup(const char *s)
{
	size_t i;

	struct {
		const char *name;
		enum order order;
	} a[] = {
		{ "prev", ORDER_PREV },
		{ "next", ORDER_NEXT }
	};

	if (s == NULL) {
		return ORDER_NEXT;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].name)) {
			return a[i].order;
		}
	}

	errno = EINVAL;
	return -1;
}

