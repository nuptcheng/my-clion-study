/************************************************************
 * @file: video55.cpp
 * @version: 1.0.0
 *
 * @brief: 三个变数的数值交换（使用if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int a, b, c, t;
  printf("Please enter three integers: ");
  scanf("%d%d%d", &a, &b, &c);
  printf("Before: %d %d %d\n", a, b, c);

  // 要做一些处理
  t = a;
  a = b;
  b = c;
  c = t;

  printf("After: %d %d %d\n", a, b, c);
  return 0;
}
