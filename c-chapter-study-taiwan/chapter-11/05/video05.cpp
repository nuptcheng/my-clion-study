/************************************************************
 * @file: video05.cpp
 * @version: 1.0.0
 *
 * @brief: 11 - 05 ｜ 有次數的韓信點兵練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int max;
  printf("MAX =  ");
  scanf("%d", &max);

  int number;
  int count = 0;
  for (number = 1; number <= max; ++number) {
    if (number % 3 == 2 && number % 5 == 3 && number % 7 == 2) {
      printf("%d\n", number);
      ++count;
    }
    if (count >= 3)
      break;
  }

  return 0;
}
