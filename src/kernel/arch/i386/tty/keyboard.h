#pragma once
#include <stdbool.h>

bool keyboard_poll_read(char *c);
void keyboard_recv(char scancode);
