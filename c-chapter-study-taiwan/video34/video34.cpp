/************************************************************
 * @file: video34.cpp
 * @version: 1.0.0
 *
 * @brief: 赋值运算
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    // int a, c;
    // // 先计算c=3，再计算a=c
    // a = c = 3;

    int a, c;
    double b, d;
    a = b = c = d = 3 + 7 / 2.;
    // a=6,c=6
    printf("a=%d,c=%d\n", a, c);
    // b=6.000000,d=6.500000
    printf("b=%f,d=%f\n", b, d);

    return 0;
}
