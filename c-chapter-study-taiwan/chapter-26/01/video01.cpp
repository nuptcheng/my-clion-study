/************************************************************
 * @file: video01.cpp
 * @version: 1.0.0
 *
 * @brief: 26 - 01 ｜ 指標與字串
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
    char strB[] = {'t','e','s','t','\0'};

    printf("%s\n", strA);  // test
    printf("%s\n", strB);  // test

    char *strC = "test";
    printf("%s\n", strC);  // test

    return 0;
}
