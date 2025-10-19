/************************************************************
 * @file: video04.cpp
 * @version: 1.0.0
 *
 * @brief: 26 - 04 ｜ 字串字面常數與 const char *
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    const char *strB = "test";
    printf("%s\n", strB);  // test
    printf("%c\n", *strB); // t

    strB = "Test";
    printf("%s\n", strB); // Test
    printf("%c\n", *strB); // T


    return 0;
}
