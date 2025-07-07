/* Define hwaddr if it exists.  */

#ifndef HWADDR_H
#define HWADDR_H


#define HWADDR_BITS 64
/* hwaddr is the type of a physical address (its size can
   be different from 'target_ulong').  */

typedef uint64_t hwaddr;
#define HWADDR_MAX UINT64_MAX
#define HWADDR_FMT_plx "%016" PRIx64
#define HWADDR_PRId PRId64
#define HWADDR_PRIi PRIi64
#define HWADDR_PRIo PRIo64
#define HWADDR_PRIu PRIu64
#define HWADDR_PRIx PRIx64
#define HWADDR_PRIX PRIX64

/**
* 定义内存映射条目结构体
* 用于描述内存区域的基地址和大小
*/
typedef struct MemMapEntry {
	// 内存区域的基地址
	hwaddr base;
	// 内存区域的大小
	hwaddr size;
} MemMapEntry;

#endif
