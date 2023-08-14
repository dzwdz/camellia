#pragma once

#define INFINITY __builtin_inff()
#define HUGE_VAL ((double)INFINITY)

double acos(double x);
double asin(double x);
double atan2(double x, double y);
double cos(double x);
double cosh(double x);
double sin(double x);
double sinh(double x);
double tan(double x);
double tanh(double x);

double fabs(double x);
double floor(double x);
double ceil(double x);
double log(double x);
double log2(double x);
double log10(double x);
double exp(double x);
double fmod(double x, double y);
double frexp(double num, int *exp);
double ldexp(double x, int exp);
double pow(double x, double y);
double sqrt(double x);
