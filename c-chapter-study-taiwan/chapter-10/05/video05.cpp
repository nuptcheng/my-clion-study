/************************************************************
 * @file: video05.cpp
 * @version: 1.0.0
 *
 * @brief: 10 - 05 ｜ 求連續整數和的練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  // 示例1：从1加到100
  int sum = 0;
  int N;
  printf("N = : ");
  scanf("%d", &N);
  for (int i = 0; i <= N; ++i) {
    sum += i;
  }
  printf("sum = %d\n", sum);
  return 0;
}
