/************************************************************
 * @file: video06.cpp
 * @version: 1.0.0
 *
 * @brief: 26 - 06 ｜ 使用函式複製字串
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>
#include <string.h>

int main() {
    // const char *source = "test";
    // // 在64位系统中，指针大小是8字节，在ARM32系统中，指针大小是4字节
    // printf("length: %d\n", sizeof(source));  // 8，得到的是指针的大小
    // printf("length: %d\n", sizeof(*source));  // 1
    // // 正确获得字符串长度的语法如下
    // printf("length: %lu\n", strlen(source));  // 4

    const char *source = "test";
    char destination[5];
    strcpy(destination, source);
    printf("new string: %s\n", destination);



    return 0;
}
