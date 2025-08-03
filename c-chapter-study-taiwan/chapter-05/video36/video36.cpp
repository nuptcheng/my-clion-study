/************************************************************
 * @file: video36.cpp
 * @version: 1.0.0
 *
 * @brief: 关系运算
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    // result is: 0
    printf("result is: %d\n", 3 > 5);
    // result is: 1
    printf("result is: %d\n", 3 < 5);
    // result is: 0
    printf("result is: %d\n", 3 == 5);
    // result is: 1
    printf("result is: %d\n", 3 != 5);
    // result is: 0
    printf("result is: %d\n", 3 >= 5);
    // result is: 1
    printf("result is: %d\n", 3 <= 5);
    // result is: 0
    printf("result is: %d\n", 3 > 2 > 1);
    return 0;
}
