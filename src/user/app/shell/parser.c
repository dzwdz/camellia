#include "shell.h"
#include <stdbool.h>
#include <string.h>

static bool isspace(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

static char skipspace(char **sp) {
	char *s = *sp;
	while (*s && isspace(*s)) s++;
	*sp = s;
	return *s;
}

static char *parg(char **sp) {
	char *s = *sp;
	char *res = NULL;

	if (skipspace(&s)) {
		switch (*s) {
			case '"':
				s++;
				res = s;
				while (*s && *s != '"')
					s++;
				break;
			default:
				res = s;
				while (*s && !isspace(*s) && *s != '>')
					s++;
				break;
		}
		if (*s) *s++ = '\0';
	}

	*sp = s;
	return res;
}

int parse(char *s, char **argv, size_t argvlen, char **redir) {
	if (argvlen == 0) return -1;
	size_t argc = 0;
	char *arg;

	*argv = NULL;
	*redir = NULL;

	while (skipspace(&s)) {
		switch (*s) {
			case '>':
				s++;
				*redir = parg(&s);
				break;
			default:
				arg = parg(&s);
				argv[argc++] = arg;
				if (argc >= argvlen)
					return -1;
		}
	}
	argv[argc] = NULL;
	return argc;
}
