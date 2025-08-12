/************************************************************
 * @file: video06.cpp
 * @version: 1.0.0
 *
 * @brief: 12 - 06 ｜ 求簡易整數方程式的練習 (使用 for 述句 )
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int i, j;
  for (i = 1; i <= 30 / 2; ++i) {
    int number_1 = i;
    int number_2 = 30 - i;
    if (number_1 * number_2 == 221) {
      printf("%d, %d\n", number_1, number_2);
    }
  }

  return 0;
}
