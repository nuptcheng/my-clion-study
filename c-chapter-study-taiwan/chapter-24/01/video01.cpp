/************************************************************
 * @file: video01.cpp
 * @version: 1.0.0
 *
 * @brief: 24 - 01 ｜ 指標與函式呼叫
 *       [1] 通过传递指针修改变量值
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

/**
 * 加1操作
 * @param n 变量n是int*类型，也就是int类型的变量起始地址
 */
void addone(int *n) {
    *n = *n + 1;
}

int main() {
    int a = 3;
    // 等同于int *aAddr = &a，然后把aAddr传入addone
    addone(&a);
    printf("a = %d\n", a);

    return 0;
}
