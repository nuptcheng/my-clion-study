/************************************************************
 * @file: video05.cpp
 * @version: 1.0.0
 *
 * @brief: 26 - 05 ｜ 指標與 const
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    char strA[] = "test";
    printf("%c\n", *strA); // t

    // hello在常量区rodata，不能修改
    char *strB = "hello";
    printf("%s\n", *strB);

    // 下行报错，因为*strB指向的内容再常量区rodata，不能修改
    // *strB = 'H';
    printf("%s\n", *strB);

    return 0;
}
