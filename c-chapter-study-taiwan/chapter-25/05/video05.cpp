/************************************************************
 * @file: video05.cpp
 * @version: 1.0.0
 *
 * @brief: 25 - 04 ｜ 指標與下標運算子
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

// int maxv(int *);
//
// int main() {
//     int a[3] = {3, 9, 7};
//     // 这里参数a等同于&a[0]，即数组a第1个元素地址
//     printf("Max：%d\n", maxv(a));
//     return 0;
// }
//
// int maxv(int *n) {
//     int max = *(n + 0), i;
//     for (i = 1; i < 3; i++) {
//         if (*(n + 1) > max) {
//             max = *(n + i);
//         }
//     }
//     return max;
// }


int maxv(int *, int);

int main() {
    int a[3] = {101, 9, 7};
    // 这里参数a等同于&a[0]，即数组a第1个元素地址
    printf("Max：%d\n", maxv(a, 3));
    int b[5] = {3, 9, 7, 108, 99};
    printf("Max：%d\n", maxv(b, 5));
    return 0;
}

int maxv(int *n, int N) {
    int max = *(n + 0);
    for (int i = 1; i < N; i++) {
        if (*(n + i) > max) {
            max = *(n + i);
        }
    }
    return max;
}
