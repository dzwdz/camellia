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

static bool isspecial(char c) {
	return c == '>' || c == '#';
}

static char *parg(char **sp) {
	char *s = *sp;
	char *res = NULL;

	if (skipspace(&s)) {
		// TODO incorrectly handles strings like a"b"
		switch (*s) {
			case '"':
				s++;
				res = s;
				while (*s && *s != '"')
					s++;
				break;
			default:
				res = s;
				while (*s && !isspace(*s) && !isspecial(*s))
					s++;
				if (*s == '#') {
					*s = '\0'; /* end parsing early */
					if (res == s) /* don't output an empty arg */
						res = NULL;
				}
				break;
		}
		if (*s) *s++ = '\0';
	}

	*sp = s;
	return res;
}

int parse(char *s, char **argv, size_t argvlen, struct redir *redir) {
	if (argvlen == 0) return -1;
	size_t argc = 0;
	char *arg;

	*argv = NULL;
	redir->stdout = NULL;
	redir->append = false;

	while (skipspace(&s)) {
		switch (*s) {
			case '>':
				s++;
				if (*s == '>') {
					s++;
					redir->append = true;
				}
				redir->stdout = parg(&s);
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
