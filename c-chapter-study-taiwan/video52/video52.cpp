/************************************************************
 * @file: video52.cpp
 * @version: 1.0.0
 *
 * @brief: 对3个变数求中位数（使用if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int a, b, c, med;
  printf("Please enter three integers: ");
  scanf("%d%d%d", &a, &b, &c);
  // 取中位数
  med = a;
  if (a <= b && b <= c || c <= b && b <= a) {
    med = b;
  }
  if (a <= c && c <= b || b <= c && c <= a) {
    med = c;
  }

  printf("The median is %d.\n ", med);
  return 0;
}
