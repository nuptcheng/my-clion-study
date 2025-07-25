/************************************************************
 * @file: video16.cpp
 * @version: 1.0.0
 *
 * @brief: 2个数字求和
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/18 10:03
 ************************************************************/

#include <stdio.h>

int main() {
    int integer1;
    int integer2;
    printf("Please enter the first integer: ");
    // &是取地址运算符
    scanf("%d", &integer1);
    printf("Please enter the second integer: ");
    scanf("%d", &integer2);
    const int sum = integer1 + integer2;
    printf("Sum is %d. \n", sum);
    return 0;
}
