/************************************************************
 * @file: video06.cpp
 * @version: 1.0.0
 *
 * @brief: 11 - 06 ｜ 質數判斷練習 (使用 for 述句)
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
  printf("Please enter a number: ");
  scanf("%d", &number);
  for (int i = 2; i < number; ++i) {
    if (number % i == 0) {
      // printf("%d\n", i);
      printf("No, not prime\n");
    }
  }
  printf("Yes, is prime\n");

  return 0;
}
