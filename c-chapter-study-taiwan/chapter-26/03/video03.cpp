/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 26 - 03 ｜ const 修飾字
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    int a = 3;
    const int b = 5;

    a = 4;
    int *c = &b;
    *c = 6;
    printf("%d\n", b);
    return 0;
}
