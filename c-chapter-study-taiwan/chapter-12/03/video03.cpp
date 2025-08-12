/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 12 - 03 ｜ 用文字繪製空心長方形的練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

/**
 * 自己想的方法
 */
// int main() {
//   int N;
//   printf("N = ");
//   scanf("%d", &N);
//
//   int i, j;
//   for (i = 1; i <= N; ++i) {
//     for (j = 1; j <= N; ++j) {
//       // 第一行和最后一行完整*
//       if (i == 1 || i == N) {
//         printf("*");
//       } else {
//         // 每一行头尾*
//         if (j == 1 || j == N) {
//           printf("*");
//         }
//         // 每一行其余空白
//         else {
//           printf(" ");
//         }
//       }
//     }
//     printf("\n");
//   }
//
//   return 0;
// }

/**
 * 视频优化方法
 *
 * @return
 */
int main() {
  int N;
  printf("N = ");
  scanf("%d", &N);

  int i, j;
  for (i = 1; i <= N; ++i) {
    for (j = 1; j <= N; ++j) {
      if (i == 1 || i == N || j == 1 || j == N) {
        printf("*");
      } else {
        printf(" ");
      }
    }
    printf("\n");
  }

  return 0;
}
