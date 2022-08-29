#include <math.h>
#include <user/lib/panic.h>

int abs(int i) {
	return i < 0 ? -i : i;
}


// TODO port a libm
#pragma GCC diagnostic ignored "-Wunused-parameter"
double acos(double x)	{ __libc_panic("unimplemented"); }
double asin(double x)	{ __libc_panic("unimplemented"); }
double atan2(double x, double y)	{ __libc_panic("unimplemented"); }
double cos(double x)	{ __libc_panic("unimplemented"); }
double cosh(double x)	{ __libc_panic("unimplemented"); }
double sin(double x)	{ __libc_panic("unimplemented"); }
double sinh(double x)	{ __libc_panic("unimplemented"); }
double tan(double x)	{ __libc_panic("unimplemented"); }
double tanh(double x)	{ __libc_panic("unimplemented"); }

double fabs(double x)	{ __libc_panic("unimplemented"); }
double floor(double x)	{ __libc_panic("unimplemented"); }
double ceil(double x)	{ __libc_panic("unimplemented"); }
double log(double x)	{ __libc_panic("unimplemented"); }
double log2(double x)	{ __libc_panic("unimplemented"); }
double log10(double x)	{ __libc_panic("unimplemented"); }
double exp(double x)	{ __libc_panic("unimplemented"); }
double fmod(double x, double y)	{ __libc_panic("unimplemented"); }
double frexp(double num, int *exp)	{ __libc_panic("unimplemented"); }
double ldexp(double x, int exp)	{ __libc_panic("unimplemented"); }
double pow(double x, double y)	{ __libc_panic("unimplemented"); }
double sqrt(double x)	{ __libc_panic("unimplemented"); }
