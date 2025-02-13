#include "types.h"
#include <stddef.h>

/* syscall */
uint64_t __SYSCALL(size_t syscall_id, reg_t arg1, reg_t arg2, reg_t arg3);

#define __NR_write 64
#define __NR_sched_yield 124
#define __NR_exit 93
#define __NR_gettimeofday 169



uint64_t sys_write(size_t fd, const char* buf, size_t len);
uint64_t sys_yield();
uint64_t sys_gettime();