#include <ctype.h>

int isalpha(int c) {
	return islower(c) || isupper(c);
}

int isalnum(int c) {
	return isalpha(c) || isdigit(c);
}

int isdigit(int c) {
	return '0' <= c && c <= '9';
}

int islower(int c) {
	return 'a' <= c && c <= 'z';
}

int isspace(int c) {
	return c == ' '
		|| c == '\f'
		|| c == '\n'
		|| c == '\r'
		|| c == '\t'
		|| c == '\v';
}

int isupper(int c) {
	return 'A' <= c && c <= 'Z';
}

