#pragma once
/* the instruction format is bound to change, atm it's extremely inefficient */

/* takes 5 arguments */
#define EXECBUF_SYSCALL 0xF0000001
/* takes 1 argument, changes %rip */
#define EXECBUF_JMP 0xF0000002
