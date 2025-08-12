#include <timeros/os.h>
#include <timeros/syscall.h>
#include <stdarg.h>
static char out_buf[1000]; // buffer for vprintf()
/**
 * @brief 格式化输出字符串到标准输出
 *
 * @param s     格式化字符串
 * @param vl    可变参数列表
 *
 * @return      返回格式化后的字符数（不包括终止空字符）
 */
static int vprintf(const char* s, va_list vl)
{
	/* 计算需要的缓冲区大小 */
	int res = _vsnprintf(NULL, -1, s, vl);

	/* 使用计算好的缓冲区大小进行格式化 */
	_vsnprintf(out_buf, res + 1, s, vl);

	/* 将格式化后的字符串写入标准输出 */
	sys_write(stdout, out_buf, res + 1);

	return res;
}


/**
 * 格式化输出字符串到标准输出
 *
 * @param s 格式化字符串，遵循C语言格式规范
 *          支持常见的格式化占位符如%d, %x, %s等
 *
 * @param ... 可变参数列表，与格式化字符串中的占位符一一对应
 *
 * @return 返回成功输出的字符数量
 */
int printf(const char* s, ...)
{
    // 用于存储格式化后的字符数，初始化为 0
    int res = 0;
    // 定义一个 va_list 类型的变量，用于处理可变参数列表
    va_list vl;
    // 初始化 va_list 变量，使其指向可变参数列表的第一个参数
    // 第二个参数 s 是可变参数列表前的最后一个固定参数
    va_start(vl, s);
    // 调用 vprintf 函数，将格式化字符串和可变参数列表传递给它
    // 并将 vprintf 返回的格式化字符数赋值给 res
    res = vprintf(s, vl);
    // 清理 va_list 变量，释放相关资源
    va_end(vl);
    // 返回格式化后的字符数
    return res;
}