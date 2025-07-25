/************************************************************
 * @file: video24.cpp
 * @version: 1.0.0
 *
 * @brief: 整数和浮点数类型的转换
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/21 08:49
 ************************************************************/


#include <stdio.h>

int main() {
    int num1, num2, num3;
    printf("Please enter the first integer: ");
    scanf("%d", &num1);
    printf("Please enter the second integer: ");
    scanf("%d", &num2);
    printf("Please enter the third integer: ");
    scanf("%d", &num3);

    // 这里改成3.就可以输出3.3333333，这里等于3.是一个double类型，整数和浮点数计算，结果还是浮点数
    // 隐式转换，因为3.的数据类型是double
    // double average = (num1 + num2 + num3) / 3.;

    // 显示转换
    double average = (num1 + num2 + num3) / (double) 3;
    printf("Average is %f", average);


    return 0;
}
