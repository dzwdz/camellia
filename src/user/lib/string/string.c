#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

long strtol(const char *restrict s, char **restrict end, int base) {
	long res = 0;
	int sign = 1;

	while (isspace(*s)) s++;

	if (*s == '+') {
		s++;
	} else if (*s == '-') {
		s++;
		sign = -1;
	}

	if (base == 0) {
		if (*s == '0') {
			s++;
			if (*s == 'x' || *s == 'X') {
				s++;
				base = 16;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
	}

	for (;;) {
		unsigned char digit = *s;
		if      ('0' <= digit && digit <= '9') digit -= '0';
		else if ('a' <= digit && digit <= 'z') digit -= 'a' - 10;
		else if ('A' <= digit && digit <= 'Z') digit -= 'A' - 10;
		else break;

		if (digit >= base) break;
		// TODO overflow check
		res *= base;
		res += digit;

		s++;
	}
	if (end) *end = (void*)s;
	return res * sign;
}

#include <bits/panic.h>
double strtod(const char *restrict s, char **restrict end) {
	(void)s; (void)end;
	__libc_panic("unimplemented");
}

char *strchr(const char *s, int c) {
	for (; *s; s++) {
		if (*s == c) return (char *)s;
	}
	return NULL;
}

char *strrchr(const char *s, int c) {
	for (int i = strlen(s) + 1; i >= 0; i--) {
		if (s[i] == c) return (char *)s + i;
	}
	return NULL;
}

size_t strspn(const char *s, const char *accept) {
	size_t l = 0;
	for (; s[l] && strchr(accept, s[l]); l++);
	return l;
}

size_t strcspn(const char *s, const char *reject) {
	size_t l = 0;
	for (; s[l] && !strchr(reject, s[l]); l++);
	return l;
}

char *strpbrk(const char *s1, const char *s2) {
	for (; *s1; s1++) {
		if (strchr(s2, *s1)) return (char*)s1;
	}
	return NULL;
}

char *strtok(char *restrict s, const char *restrict sep) {
	static char *state;
	return strtok_r(s, sep, &state);
}

char *strtok_r(char *restrict s, const char *restrict sep, char **restrict state) {
	char *end;
	if (!s) s = *state;
	s += strspn(s, sep); /* beginning of token */
	if (!*s) return NULL;

	end = s + strcspn(s, sep);
	if (*end) {
		*end = '\0';
		*state = end + 1;
	} else {
		*state = end;
	}
	return s;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (s1[i] < s2[i]) return -1;
		if (s1[i] > s2[i]) return  1;
	}
	return 0;
}

int strcoll(const char *s1, const char *s2) {
	return strcmp(s1, s2);
}

// TODO implement strstr using Boyer-Moore
char *strstr(const char *s1, const char *s2) {
	size_t l1 = strlen(s1), l2 = strlen(s2);
	for (; l2 <= l1; s1++, l1--) {
		if (memcmp(s1, s2, l2) == 0) return (char*)s1;
	}
	return NULL;
}

char *strcpy(char *restrict s1, const char *restrict s2) {
	while (*s2) *s1++ = *s2++;
	*s1 = *s2;
	return s1;
}

char *strncpy(char *restrict s1, const char *restrict s2, size_t n) {
	for (size_t i = 0; i < n; i++) {
		s1[i] = s2[i];
		if (s1[i] == '\0') return s1 + i; // TODO fill with null bytes
	}
	return s1 + n;
}

char *strdup(const char *s) {
	size_t len = strlen(s) + 1;
	char *buf = malloc(len);
	if (buf) memcpy(buf, s, len);
	return buf;
}

/* strings.h */
int strcasecmp(const char *s1, const char *s2) {
	return strncasecmp(s1, s2, ~0);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
	for (size_t i = 0; i < n; i++) {
		char c1 = tolower(s1[i]), c2 = tolower(s2[i]);
		if (c1 == '\0' || c1 != c2) {
			if (c1 < c2)      return -1;
			else if (c1 > c2) return 1;
			else              return 0;
		}
	}
	return 0;
}
