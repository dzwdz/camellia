#pragma once
#include <stdbool.h>
#include <stdint.h>

bool keyboard_poll_read(char *c);
void keyboard_recv(uint8_t scancode);
