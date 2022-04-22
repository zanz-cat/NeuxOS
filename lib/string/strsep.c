#include <string.h>

char *strsep(char **sp, const char *delim)
{
	char *s = *sp;
	char *p;

	p = NULL;
	if (s && *s && (p = strpbrk(s, delim))) {
		*p++ = 0;
	}

	*sp = p;
	return s;
}