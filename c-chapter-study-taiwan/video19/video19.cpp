/************************************************************
 * @file: video19.cpp
 * @version: 1.0.0
 *
 * @brief:数值交换
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/19 16:30
 ************************************************************/


#include <stdio.h>


int main() {
    int integer1, integer2;
    printf("Please enter the first integer: ");
    scanf("%d", &integer1);
    printf("Please enter the second integer: ");
    scanf("%d", &integer2);

    /*方法1*/
    // int temp = integer1;
    // integer1 = integer2;
    // integer2 = temp;

    /*方法2*/
    integer1 = integer1 ^ integer2;
    integer2 = integer1 ^ integer2;
    integer1 = integer1 ^ integer2;


    printf("integer1: %d\n", integer1);
    printf("integer2: %d\n", integer2);
    return 0;
}
