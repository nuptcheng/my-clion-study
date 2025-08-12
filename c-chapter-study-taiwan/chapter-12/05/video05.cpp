/************************************************************
 * @file: video05.cpp
 * @version: 1.0.0
 *
 * @brief: 12 - 05 ｜ 基於座標法用文字繪製三角形 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int N;
  printf("N = ");
  scanf("%d", &N);

  int i, j;
  for (i = 1; i <= N; ++i) {
    for (j = 1; j <= N; ++j) {
      // 第1行和最后1行全是*
      if (j == 1 || i == N || i == j) {
        printf("*");
      } else {
        printf(" ");
      }
    }
    printf("\n");
  }
  return 0;
}
