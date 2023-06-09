#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

char *strncpy(char *restrict dst, const char *restrict src, size_t n) {
	for (size_t i = 0; i < n; i++) {
		dst[i] = src[i];
		if (dst[i] == '\0') return dst + i; // TODO fill with null bytes
	}
	return dst;
}

char *stpncpy(char *restrict dst, const char *restrict src, size_t n) {
	return stpncpy(dst, src, n) + n;
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
