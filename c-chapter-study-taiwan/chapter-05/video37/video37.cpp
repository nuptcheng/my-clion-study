/************************************************************
 * @file: video37.cpp
 * @version: 1.0.0
 *
 * @brief: 逻辑运算
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    // 1
    printf("the result is %d\n", 3 > 2 && 2 > 1);
    // 0
    printf("the result is %d\n", 2 > 3 && 2 > 1);
    // 0
    printf("the result is %d\n", !(3 > 2));
    // 1
    printf("the result is %d\n", !(1 > 2));
    // 0
    printf("the result is %d\n", !3 > 2);
    return 0;
}
