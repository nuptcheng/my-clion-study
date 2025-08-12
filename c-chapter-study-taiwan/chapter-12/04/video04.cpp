/************************************************************
 * @file: video04.cpp
 * @version: 1.0.0
 *
 * @brief: 12 - 04 ｜ 用文字繪製三角形的練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

/**
 * 需求1：实心三角形实现方式
 *
 * @return
 */
// int main() {
//   int N;
//   printf("N = ");
//   scanf("%d", &N);
//
//   for (int i = 1; i <= N; ++i) {
//     for (int j = 1; j <= i; j++) {
//       printf("*");
//     }
//     printf("\n");
//   }
//
//   return 0;
// }

/**
 * 需求2：空心三角形实现方式
 *
 * @return
 */
int main() {
  int N;
  printf("N = ");
  scanf("%d", &N);

  for (int i = 1; i <= N; ++i) {
    for (int j = 1; j <= i; j++) {
      if (i == 1 || i == N || j == 1 || j == i) {
        printf("*");
      } else {
        printf(" ");
      }
    }
    printf("\n");
  }

  return 0;
}
