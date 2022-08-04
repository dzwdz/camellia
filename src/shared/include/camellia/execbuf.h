#pragma once
/* the instruction format is bound to change, atm it's extremely inefficient */

#define EXECBUF_MAX_LEN 4096

/* takes 6 arguments */
#define EXECBUF_SYSCALL 0xF0000001
/* takes 1 argument, changes %rip */
#define EXECBUF_JMP 0xF0000002
