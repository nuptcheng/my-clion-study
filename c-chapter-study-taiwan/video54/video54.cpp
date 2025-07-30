/************************************************************
 * @file: video54.cpp
 * @version: 1.0.0
 *
 * @brief: 对两个变数依照大小排序（使用if语句）
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
  printf("Please enter two integers: ");
  scanf("%d%d", &a, &b);
  printf("Before: %d %d\n", a, b);

  // 要做一些处理
  if (a > b) {
    int temp = a;
    a = b;
    b = temp;
  }
  printf("After: %d %d\n", a, b);
  return 0;
}
