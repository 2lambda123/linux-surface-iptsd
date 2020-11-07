/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTSD_SYSCALL_H_
#define _IPTSD_SYSCALL_H_

#include <stdint.h>

#define iptsd_err(ERRNO, ARGS...) \
	iptsd_utils_err(ERRNO, __FILE__, __LINE__, ##ARGS)

int iptsd_utils_open(const char *file, int flags);
int iptsd_utils_close(int fd);
int iptsd_utils_read(int fd, void *buf, size_t count);
int iptsd_utils_write(int fd, void *buf, size_t count);
int iptsd_utils_ioctl(int fd, unsigned long request, void *data);
void iptsd_utils_err(int err, const char *file,
	int line, const char *format, ...);
uint64_t iptsd_utils_msec_timestamp(void);

#endif /* _IPTSD_SYSCALL_H_ */
