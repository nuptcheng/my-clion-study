/************************************************************
 * @file: video48.cpp
 * @version: 1.0.0
 *
 * @brief: 对两个变数求最大值（if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int a, b;
  printf("Please enter the first integer: ");
  scanf("%d", &a);
  printf("Please enter the second integer: ");
  scanf("%d", &b);

  int max = a;
  if (max < b) {
    max = b;
  }

  printf("The maxinum is %d.\n", max);
  return 0;
}
