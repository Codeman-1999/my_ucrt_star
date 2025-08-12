#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>  

#define TARGET_PATH "../user/bin/"

/**
 * @brief 用于比较两个字符串的函数，供 qsort 函数使用。
 * 
 * 该函数遵循 qsort 所需的比较函数原型，用于比较两个字符串指针。
 * 它会将传入的指针转换为字符指针的指针，然后解引用得到字符指针，
 * 最后使用 strcmp 函数进行字符串比较。
 * 
 * @param a 指向第一个字符串指针的指针，类型为 const void*。
 * @param b 指向第二个字符串指针的指针，类型为 const void*。
 * @return int 如果第一个字符串小于第二个字符串，返回负值；
 *             如果两个字符串相等，返回 0；
 *             如果第一个字符串大于第二个字符串，返回正值。
 */
int compare_strings(const void* a, const void* b) {
    // 将 const void* 类型的指针转换为 const char** 类型，再解引用得到 const char* 类型的字符串指针
    // 然后使用 strcmp 函数比较这两个字符串
    return strcmp(*(const char**)a, *(const char**)b);
}

/**
 * @brief 插入应用程序数据到汇编文件 src/link_app.S 中
 * 
 * 该函数会读取 ./user/bin 目录下的所有文件，提取文件名（去除扩展名），
 * 对文件名进行排序，然后将应用程序数量、应用程序起始结束地址和应用程序名
 * 写入到 src/link_app.S 文件中。
 */
void insert_app_data() {
    // 打开 src/link_app.S 文件以写入模式
    FILE* f = fopen("src/link_app.S", "w");
    // 检查文件是否成功打开
    if (f == NULL) {
        // 若打开失败，输出错误信息
        perror("Failed to open file");
        // 程序异常退出
        exit(EXIT_FAILURE);
    }
    
    // 定义一个字符指针数组，用于存储应用程序名，假设最多 100 个应用
    char* apps[100]; 
    // 记录应用程序的数量
    int app_count = 0;

    // 打开 ./user/bin 目录
    DIR* dir = opendir("./user/bin");
    // 检查目录是否成功打开
    if (dir == NULL) {
        // 若打开失败，输出错误信息
        perror("Failed to open directory");
        // 程序异常退出
        exit(EXIT_FAILURE);
    }
    
    // 用于存储目录项信息的结构体指针
    struct dirent* dir_entry;
    // 循环读取目录中的每一项
    while ((dir_entry = readdir(dir)) != NULL) {
        // 获取当前目录项的文件名
        char* name_with_ext = dir_entry->d_name;
        
        // 排除掉 . 和 .. 条目，这两个条目分别代表当前目录和上级目录
        if (name_with_ext[0] == '.' && (name_with_ext[1] == '\0' || (name_with_ext[1] == '.' && name_with_ext[2] == '\0'))) {
            // 跳过当前条目
            continue; 
        }
        
        // 获取文件名的长度
        int len = strlen(name_with_ext);

        // 去除文件扩展名，通过将第一个点替换为字符串结束符
        for (int i = 0; i < len; i++) {
            if (name_with_ext[i] == '.') {
                name_with_ext[i] = '\0';
                break;
            }
        }
        
        // 使用 strdup 函数复制文件名，返回指向新字符串的指针
        apps[app_count] = strdup(name_with_ext);
        // 应用程序数量加 1
        app_count++;
        // 输出当前处理的文件名和应用程序数量
        printf("File name: %s, app_count: %d\n", name_with_ext, app_count);
    }
    
    // 关闭目录
    closedir(dir);
    
    // 对存储应用程序名的数组进行排序
    qsort(apps, app_count, sizeof(char*), compare_strings);

    // 向 src/link_app.S 文件写入应用程序数量信息
    fprintf(f, "\n.align 3\n.section .data\n.global _num_app\n_num_app:\n.quad %d", app_count);
    
    // 向 src/link_app.S 文件写入每个应用程序的起始地址信息
    for (int i = 0; i < app_count; i++) {
        fprintf(f, "\n.quad app_%d_start", i);
    }
    // 向 src/link_app.S 文件写入最后一个应用程序的结束地址信息
    fprintf(f, "\n.quad app_%d_end", app_count - 1);

    // 向 src/link_app.S 文件写入应用程序名信息的起始标记
    fprintf(f,"\n.global _app_names\n_app_names:");
    // 向 src/link_app.S 文件写入每个应用程序的名称
    for (int i = 0; i < app_count; i++) 
    {
        fprintf(f,"\n.string \"%s\"",apps[i]);
    }
    
    // 遍历每个应用程序名
    for (int i = 0; i < app_count; i++) {
        // 输出当前应用程序的编号和名称
        printf("app_%d: %s\n", i, apps[i]);
        // 向 src/link_app.S 文件写入每个应用程序的起始和结束地址信息，并包含二进制文件内容
        fprintf(f, "\n.section .data\n.global app_%d_start\n.global app_%d_end\n.align 3\napp_%d_start:\n.incbin \"%s%s\"\napp_%d_end:", i, i, i, TARGET_PATH, apps[i], i);
        // 释放之前使用 strdup 分配的内存
        free(apps[i]);
    }
    
    // 关闭文件
    fclose(f);
}


int main() {
    insert_app_data();
    return 0;
}
