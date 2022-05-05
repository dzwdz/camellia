#pragma once
#include <kernel/vfs/request.h>
#include <stdbool.h>
#include <stddef.h>

void	serial_init(void);

bool	serial_ready(void);
void	serial_irq(void);
size_t	serial_read(char *buf, size_t len);

void	serial_write(const char *buf, size_t len);

void  vfs_com1_accept(struct vfs_request *);
bool vfs_com1_ready(struct vfs_backend *);
