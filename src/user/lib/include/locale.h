#pragma once
#include <limits.h>

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MESSAGES 3
#define LC_MONETARY 4
#define LC_NUMERIC 5
#define LC_TIME 6

struct lconv {
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char *currency_symbol;
	char frac_digits;
	char p_cs_precedes;
	char n_cs_precedes;
	char p_sep_by_space;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
	char *int_curr_symbol;
	char int_frac_digits;
	char int_p_cs_precedes;
	char int_n_cs_precedes;
	char int_p_sep_by_space;
	char int_n_sep_by_space;
	char int_p_sign_posn;
	char int_n_sign_posn;
};

static inline struct lconv *localeconv(void) {
	/* per Linux's lconv(3) */
	static struct lconv locale = (struct lconv){
		.decimal_point = ".",
		.thousands_sep = "",
		.grouping = "",
		.mon_decimal_point = "",
		.mon_thousands_sep = "",
		.mon_grouping = "",
		.positive_sign = "",
		.negative_sign = "",
		.currency_symbol = "",
		.frac_digits = CHAR_MAX,
		.p_cs_precedes = CHAR_MAX,
		.n_cs_precedes = CHAR_MAX,
		.p_sep_by_space = CHAR_MAX,
		.n_sep_by_space = CHAR_MAX,
		.p_sign_posn = CHAR_MAX,
		.n_sign_posn = CHAR_MAX,
		.int_curr_symbol = "",
		.int_frac_digits = CHAR_MAX,
		.int_p_cs_precedes = CHAR_MAX,
		.int_n_cs_precedes = CHAR_MAX,
		.int_p_sep_by_space = CHAR_MAX,
		.int_n_sep_by_space = CHAR_MAX,
		.int_p_sign_posn = CHAR_MAX,
		.int_n_sign_posn = CHAR_MAX,
	};
	return &locale;
}

char *setlocale(int category, const char *locale);
