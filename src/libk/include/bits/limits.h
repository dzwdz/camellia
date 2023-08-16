/** Adapted from code (c) 2023 Pedro Falcato, with permission.
 * Originally licensed under BSD-2-Clause-Patent.
 */

#if defined (__LP64__)
#define __LIMITS_64BIT
#endif

#define CHAR_BIT 8

#define SCHAR_MIN -128
#define SCHAR_MAX 127
#define UCHAR_MAX 255

#if '\xff' < 0
/* char is signed */
#  define CHAR_MIN  SCHAR_MIN
#  define CHAR_MAX  SCHAR_MAX
#else
/* char is unsigned */
#  define CHAR_MIN  0
#  define CHAR_MAX  UCHAR_MAX
#endif

#define SHRT_MIN   (-1 - 0x7fff)
#define SHRT_MAX   0x7fff
#define USHRT_MAX  0xffff

#define INT_MIN   (-1 - 0x7fffffff)
#define INT_MAX   0x7fffffff
#define UINT_MAX  0xffffffffU

#ifdef __LIMITS_64BIT
#  define LONG_MAX   0x7fffffffffffffffL
#  define LONG_MIN   (-1 - 0x7fffffffffffffffL)
#  define ULONG_MAX  0xffffffffffffffffUL
#else
#  define LONG_MAX   0x7fffffffL
#  define LONG_MIN   (-1 - 0x7fffffffL)
#  define ULONG_MAX  0xffffffffUL
#endif

#define LLONG_MIN   (-1 - 0x7fffffffffffffffLL)
#define LLONG_MAX   0x7fffffffffffffffLL
#define ULLONG_MAX  0xffffffffffffffffULL
