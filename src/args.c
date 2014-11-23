#define _XOPEN_SOURCE 700

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "args.h"

static int
unescapec(const char *s, size_t *n)
{
	assert(s != NULL);
	assert(n != NULL);

	if (*s != '\\') {
		*n = 1;
		return *s;
	}

	*n = 2;

	switch (s[1]) {
	case '\\': return '\\';
	case '\"': return '\"';
	case '\'': return '\'';
	case 'f':  return '\f';
	case 'n':  return '\n';
	case 'r':  return '\r';
	case 't':  return '\t';
	case 'v':  return '\v';
	case ' ':  return ' ';

	case 'o': {
		unsigned long u;
		char *e;

		assert(strlen(s) >= 2);

		errno = 0;

		u = strtoul(s + 1, &e, 8);

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			fprintf(stderr, "octal escape %s out of range: expected \\0..\\%o inclusive",
				s, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if (u == ULONG_MAX && errno != 0) {
			fprintf(stderr, "%s: %s: expected \\0..\\%o inclusive",
				s, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		*n = e - s;
		return (unsigned char) u;
	}

	case 'x': {
		unsigned long u;
		char *e;

		assert(strlen(s) >= 3);

		errno = 0;

		u = strtoul(s + 2, &e, 16);

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			fprintf(stderr, "hex escape %s out of range: expected \\x0..\\x%x inclusive",
				s, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if (u == ULONG_MAX && errno != 0) {
			fprintf(stderr, "%s: %s: expected \\x0..\\x%x inclusive",
				s, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		*n = e - s;
		return (unsigned char) u;
	}

	default:
		/* TODO: handle error */
		return '\0';
	}
}

static int
uqstr(const char **src, char **dst, const char *end)
{
	size_t n;

	assert(src != NULL && *src != NULL);
	assert(dst != NULL && *dst != NULL);
	assert(end != NULL);

	while (**src != '\0') {
		int c;

		if (strchr(end, **src)) {
			break;
		}

		c = unescapec(*src, &n);
		if (c == -1) {
			return -1;
		}

		**dst = c;

		*dst += 1;
		*src += n;
	}

	**dst = '\0';

	return 0;
}

static int
dqstr(const char **src, char **dst)
{
	assert(src != NULL && *src != NULL);
	assert(dst != NULL && *dst != NULL);

	if (**src != '"') {
		return -1;
	}

	*src += 1;

	if (-1 == uqstr(src, dst, "\"")) {
		return -1;
	}

	if (**src != '"') {
		return -1;
	}

	*src += 1;

	**dst = '\0';

	return 0;
}

static int
sqstr(const char **src, char **dst)
{
	size_t n;

	assert(src != NULL && *src != NULL);
	assert(dst != NULL && *dst != NULL);

	if (**src != '\'') {
		return -1;
	}

	*src += 1;

	n = strcspn(*src, "'");
	memmove(*dst, *src, n);

	*src += n;
	*dst += n;
	**dst = '\0';

	if (**src != '\'') {
		return -1;
	}

	*src += 1;

	return 0;
}

/*
 * Split a string of arguments to a NULL-terminated argv array.
 *
 * The syntax parsed is intended to be in much the spirit of sh(1).
 * Each argument is a sequence of one or more adjacent double-quoted
 * strings, single-quoted strings or unquoted sequences of characters
 * (outside of strings).
 *
 * Both double-quoted and unquoted characters may contain C-style
 * escape sequences. Whitespace may also be escaped in an unquoted sequence.
 *
 * The destination must point to storage at least large enough to hold the
 * original string. The destination may point to the same location as the
 * source, but not otherwise overlap src.
 *
 * Returns the number of elements set in argv[] (not including the
 * terminating NULL), or -1 on error.
 */
int
args(const char *src, char *dst, char *argv[], int count)
{
	int argc;

	assert(src != NULL);
	assert(dst != NULL);

	if (count <= 0) {
		errno = EINVAL;
		return -1;
	}

	src += strspn(src, " \t\v\f\r\n");

	argc = (*src != '\0');

	if (argv != NULL) {
		argv[0] = dst;
	}

	do {
		int r;

		switch (*src) {
		case '\t': case '\v': case '\f':
		case '\r': case '\n': case ' ':
			src += strspn(src, " \t\v\f\r\n");

			if (*src != '\0') {
				*dst++ = '\0';
				argc++;
			}

		case '\0':
			if (*src == '\0' || argv == NULL) {
				continue;
			}

			if (argc - 1 >= count) {
				errno = ENOMEM;
				return -1;
			}

			argv[argc - 1] = dst;
			continue;

		case '\'': r = sqstr(&src, &dst); break;
		case '\"': r = dqstr(&src, &dst); break;

		default:
			r = uqstr(&src, &dst, " \t\v\f\r\n\'\"");
			break;
		}

		if (r == -1) {
			return -1;
		}

	} while (*src != '\0');

	if (argv != NULL) {
		if (argc >= count) {
			errno = ENOMEM;
			return -1;
		}

		argv[argc] = NULL;
	}

	return argc;
}

int
args_count(char *argv[])
{
	int i;

	assert(argv != NULL);

	for (i = 0; argv[i] != NULL; i++)
		;

	return i;
}

char **
args_clone(char *argv[])
{
	char **new;
	int i, n;

	assert(argv != NULL);

	n = args_count(argv);

	new = malloc((n + 1) * sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	for (i = 0; i < n; i++) {
		new[i] = strdup(argv[i]);
	}

	for (i = 0; i < n; i++) {
		if (new[i] == NULL) {
			goto error;
		}
	}

	new[i] = NULL;

	return new;

error:

	for (i = 0; i < n; i++) {
		free(new[i]);
	}

	free(new);

	return NULL;
}


#ifdef TEST_ARGS

#include <stdio.h>
#include <ctype.h>

static int
test(const char *name, int f(const char *, char *, char *[], int),
	const char *src, char *dst, size_t dstsz,
	char *argv[], size_t count,
	int eargc, const char *eargv[])
{
	int r;
	int i;
	int v;

	assert(f != NULL);
	assert(dst != NULL);

	memset(dst, (unsigned char) 0xff, dstsz);
	memset(dst, (unsigned char) 0xee, strlen(src) + 1);
	memset(argv, '_', count * sizeof argv);

	r = f(src, dst, argv, count);

	v = r == eargc;

	if (r != -1) {
		for (i = 0; v && i < r; i++) {
			v &= 0 == strcmp(argv[i], eargv[i]);
		}

		v &= argv[i] == NULL;
	}

	printf("%s(\"%s\")%.*s ",
		name, src, 10 - (int) strlen(src), "          ");

	{
		size_t i;

		printf("[");

		for (i = 0; i < dstsz; i++) {
			if (ispunct((signed char) dst[i]) || isalnum((signed char) dst[i]) || dst[i] == ' ') {
				printf(" %c ", dst[i]);
			} else {
				printf("%02x ", (unsigned char) dst[i]);
			}
		}

		printf("]");
	}

	printf(" -> %2d ", r);

	{
		printf("{ ");
		for (i = 0; i < r; i++) {
			printf("\"%s\" ", argv[i]);
		}
		printf("}");
	}

	if (!v) {
		printf(" XXX %d { ", eargc);
		for (i = 0; i < eargc; i++) {
			printf("\"%s\" ", eargv[i]);
		}
		printf("}");
	}
	printf("\n");

	return !v;
}

int main(void) {
	size_t i;
	int v;

	struct {
		const char *src;
		int eargc;
		const char *eargv[10];
	} a[] = {
		{ "\"",       -1, { NULL          } },
		{ "\'",       -1, { NULL          } },
		{ "\"abc",    -1, { NULL          } },
		{ "\'abc",    -1, { NULL          } },
		{ "",          0, { NULL          } },
		{ "  ",        0, { NULL          } },
		{ "x",         1, { "x"           } },
		{ "x y z",     3, { "x", "y", "z" } },
		{ "xyz abc",   2, { "xyz", "abc"  } },
		{ "xyz ",      1, { "xyz"         } },
		{ " xyz",      1, { "xyz"         } },
		{ " xyz ",     1, { "xyz"         } },
		{ "x y ",      2, { "x", "y"      } },
		{ " x y",      2, { "x", "y"      } },
		{ " x y ",     2, { "x", "y"      } },
		{ "\"a b\"",   1, { "a b"         } },

		{ "a\\ b",     1, { "a b"         } },
		{ "\\\"",      1, { "\""          } },
		{ "\\x41 a",   2, { "A", "a"      } },
		{ "a \\x41",   2, { "a", "A"      } },
		{ "\"\\\\ \"", 1, { "\\ "         } },
		{ "x\\\"y",    1, { "x\"y"        } },
		{ "x\\\\y",    1, { "x\\y"        } },
		{ "x\\\\",     1, { "x\\"         } },
		{ "x\\\\y",    1, { "x\\y"        } },
		{ "x\\' y",    2, { "x\'", "y"    } },
		{ " \t x\ty ", 2, { "x", "y"      } },
		{ "\\ ",       1, { " "           } },
		{ " \\ ",      1, { " "           } },
		{ " \\  ",     1, { " "           } },
		{ "\"a\"bc",   1, { "abc"         } },
		{ "ab\"c\"",   1, { "abc"         } },
		{ "\"\\\"\"b", 1, { "\"b"         } },
		{ "a \\x01x",  2, { "a", "\1x"    } },
		{ "a b\\0c",   2, { "a", "b"      } },
		{ "a \\0c",    2, { "a", ""       } },
		{ "a \\0",     2, { "a", ""       } },
		{ "a \\x00x",  2, { "a", ""       } },
		{ "a b\\0c d", 3, { "a", "b", "d" } },
		{ "a \\0c d",  3, { "a", "", "d"  } },
		{ "a \\0 d",   3, { "a", "", "d"  } },
		{ "a \\x00xd", 2, { "a", ""       } },

		{ " abc def ", 2, { "abc", "def"  } },
		{ "'a''b''c'", 1, { "abc"         } },
		{ "'a''''c'",  1, { "ac"          } },
		{ "'a'\"b\"",  1, { "ab"          } },
		{ "\"a\"'b'",  1, { "ab"          } },
		{ "'a\\'",     1, { "a\\"         } },
		{ "'a\\tc'",   1, { "a\\tc"       } },
		{ "'a\\c'",    1, { "a\\c"        } },
		{ "'a\\x41b'", 1, { "a\\x41b"     } }
	};

	struct {
		const char *src;
		int count;
		int eargc;
		const char *eargv[10];
	} b[] = {
		{ "",    0, -1, { NULL } },
		{ "",    1,  0, { NULL } },
		{ "",    1,  0, { NULL } },
		{ "",    2,  0, { NULL } },
		{ " ",   0, -1, { NULL } },
		{ " ",   1,  0, { NULL } },
		{ " ",   1,  0, { NULL } },
		{ " ",   2,  0, { NULL } },
		{ "x",   0, -1, { "x"  } },
		{ "x",   1, -1, { "x"  } },
		{ "x",   2,  1, { "x"  } },
		{ "x",   3,  1, { "x"  } },
		{ "x ",  1, -1, { "x"  } },
		{ "x ",  2,  1, { "x"  } },
		{ "x ",  3,  1, { "x"  } },
		{ " x",  1, -1, { "x"  } },
		{ " x",  2,  1, { "x"  } },
		{ " x",  3,  1, { "x"  } },
		{ " x ", 1, -1, { "x"  } },
		{ " x ", 2,  1, { "x"  } },
		{ " x ", 3,  1, { "x"  } },
		{ "x y", 0, -1, { "x", "y" } },
		{ "x y", 1, -1, { "x", "y" } },
		{ "x y", 2, -1, { "x", "y" } },
		{ "x y", 3,  2, { "x", "y" } }
	};

	v = 0;

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		char dst[10];
		char *argv[20];

		v &= !test("args", args, a[i].src, dst, sizeof dst,
			argv, sizeof argv / sizeof *argv,
			a[i].eargc, a[i].eargv);
	}

	for (i = 0; i < sizeof b / sizeof *b; i++) {
		char dst[10];
		char *argv[20];

		v &= !test("args", args, b[i].src, dst, sizeof dst,
			argv, b[i].count,
			b[i].eargc, b[i].eargv);
	}

	return v;
}

#endif

