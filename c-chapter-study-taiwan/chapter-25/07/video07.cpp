/************************************************************
 * @file: video07.cpp
 * @version: 1.0.0
 *
 * @brief: 25 - 07 ｜ 指標與遞增遞減運算子
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
//     for (int i = 0; i < 5; i++) {
//         printf("v[%i] = %i\n", i, v[i]);
//     }
//     printf("Set all elements in array to zero!\n");
//     int *p;
//     // p != &v[5]等同于p != v+5
//     for (p = v; p != &v[5]; p++) {
//         *p = 0;
//     }
//     for (int i = 0; i < 5; i++) {
//         printf("v[%i] = %i\n", i, v[i]);
//     }
//
//     return 0;
// }

int main() {
    int v[5] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        printf("v[%i] = %i\n", i, v[i]);
    }
    printf("Set all elements in array to zero!\n");
    // int *p;
    // p != &v[5]等同于p != v+5
    // for (p = v; p != &v[5]; p++) {
    //     *p = 0;
    // }

    int *p = v;
    while (p != v + 5) {
        // 等同于*p++ = 0，*p++等于*(p++)
        *p = 0;
        p++;
    }


    for (int i = 0; i < 5; i++) {
        printf("v[%i] = %i\n", i, v[i]);
    }

    return 0;
}
