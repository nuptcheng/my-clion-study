/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 25 - 03 ｜ 循序存取陣列元素 (使用指標)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

// int main() {
//     int v[5] = {1, 2, 3, 4, 5};
//     int *n = v;
//     for (int i = 0; i < 5; i++) {
//         printf("v[%d] = %d\n", i, *(n + i));
//     }
//
//     return 0;
// }


int main() {
    int v[5] = {1, 2, 3, 4, 5};
    int *n;
    for (n = v; n != v + 5; n++) {
        printf("%d\n", *n);
    }

    return 0;
}
