/************************************************************
 * @file: video03.cpp
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

void sort(int *, int *);

void swap(int *, int *);

int main() {
    int a, b;
    printf("Please enter two integers: ");
    scanf("%d%d", &a, &b);
    sort(&a, &b);
    printf("The sorted integers are: \n");
    printf("min: %d\n", a);
    printf("max: %d\n", b);
    return 0;
}

void sort(int *a, int *b) {
    if (*a > *b) {
        swap(a, b);
    }
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}
