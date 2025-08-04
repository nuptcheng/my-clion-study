/************************************************************
 * @file: video78.cpp
 * @version: 1.0.0
 *
 * @brief: 10 - 02 ｜ 數數字練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int sum = 0;
  int N;
  printf("please enter the number: ");
  scanf("%d", &N);
  for (int count = 0; count <= N; count++) {
    printf("%d\n", count);
    sum += count;
  }
  printf("The sum is %d\n", sum);
  return 0;
}
