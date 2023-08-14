/* Adapted from https://github.com/skeeto/getopt by Chris Wellons */

/* A minimal POSIX getopt() implementation in ANSI C
 *
 * This is free and unencumbered software released into the public domain.
 *
 * This implementation supports the convention of resetting the option
 * parser by assigning optind to 0. This resets the internal state
 * appropriately.
 *
 * Ref: http://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html
 */
#ifndef GETOPT_H
#define GETOPT_H

extern int optind, opterr, optopt;
extern char *optarg;

int getopt(int argc, char * const argv[], const char *optstring);

#endif
