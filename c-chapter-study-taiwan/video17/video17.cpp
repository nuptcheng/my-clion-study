/************************************************************
 * @file: video17.cpp
 * @version: 1.0.0
 *
 * @brief: 3个数字求和
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/18 10:03
 ************************************************************/

#include <stdio.h>

int main() {
    int integer;
    printf("Please enter the first integer: ");
    scanf("%d", &integer);
    int sum = integer;
    printf("Please enter the second integer: ");
    scanf("%d", &integer);
    sum = sum + integer;
    printf("Please enter the third integer: ");
    scanf("%d", &integer);
    sum = sum + integer;
    printf("Sum is %d. \n", sum);
    return 0;
}
