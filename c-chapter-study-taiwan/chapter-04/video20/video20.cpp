/************************************************************
 * @file: video20.cpp
 * @version: 1.0.0
 *
 * @brief: 数据类型
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/19 22:44
 ************************************************************/

#include <stdio.h>

int main() {
    // 2 byte
    printf("sizeof(short int) = %d byte\n: ", sizeof(short int));
    // 4 byte
    printf("sizeof(int) = %d byte\n: ", sizeof(int));
    // 8 byte
    printf("sizeof(long int) = %d byte\n: ", sizeof(long int));
    // 1 byte
    printf("sizeof(char) = %d byte\n: ", sizeof(char));
    // 4 byte
    printf("sizeof(float) = %d byte\n: ", sizeof(float));
    // 8 byte
    printf("sizeof(double) = %d byte\n: ", sizeof(double));

    return 0;
}
