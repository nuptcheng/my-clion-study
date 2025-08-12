/************************************************************
 * @file: video02.cpp
 * @version: 1.0.0
 *
 * @brief: 12 - 02 ｜ 用文字繪製長方形的練習 (使用 for 述句)
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

  for (int k = 1; k <= N; k++) {
    for (int l = 1; l <= N; l++) {
      printf("*");
    }
    printf("\n");
  }

  return 0;
}
