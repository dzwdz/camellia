#pragma once

void sysexit(void (*fun)(), void *stack_top);
void sysenter_setup();
