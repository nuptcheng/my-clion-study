/************************************************************
 * @file: video01.cpp
 * @version: 1.0.0
 *
 * @brief: 11 - 01 ｜ 找出所有正因數的練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int number;
  printf("Please enter number：");
  scanf("%d", &number);
  for (int i = 0; i <= number; ++i) {
    if (number % i == 0) {
      printf("正因数：%d\n", i);
    }
  }

  return 0;
}
