/************************************************************
 * @file: video04.cpp
 * @version: 1.0.0
 *
 * @brief: 14 - 04 ｜ 求上樓梯方法數的練習 (使用函式)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>
int fn(int);

int main() {
  int N;
  printf("Please enter a ladder number: ");
  scanf("%d", &N);
  printf("The number of ladder methods is: %d\n", fn(N));
  return 0;
}

int fn(int n) {
  if (n <= 2) {
    return n;
  }
  return fn(n - 1) + fn(n - 2);
}
