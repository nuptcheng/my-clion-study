/************************************************************
 * @file: video42.cpp
 * @version: 1.0.0
 *
 * @brief: 满额折扣计算练习（if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    int number;
    printf("Please enter the number of customers: ");
    scanf("%d", &number);
    int total = 300 * number;
    if (total > 3000) {
        total = total * 0.8;
    }
    printf("Total: %d\n", total);
    return 0;
}
