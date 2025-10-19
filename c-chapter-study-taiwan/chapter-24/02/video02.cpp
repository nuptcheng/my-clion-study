/************************************************************
 * @file: video02.cpp
 * @version: 1.0.0
 *
 * @brief: 24 - 02 ｜ 兩個變數數值交換 (使用函式)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

// 声明
void swap(int *x, int *y);

int main() {
    int a = 3, b = 5;
    swap(&a, &b);
    printf("a: %d\n", a);
    printf("b: %d\n", b);
    return 0;
}

void swap(int *x, int *y) {
    int t = *x;
    *x = *y;
    *y = t;
}
