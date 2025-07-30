/************************************************************
 * @file: video64.cpp
 * @version: 1.0.0
 *
 * @brief: 对多个变数求最大值（if-else语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int a, b, c;
  printf("Please enter three integers: ");
  scanf("%d%d%d", &a, &b, &c);
  if (a >= b && a >= c) {
    printf("The maximum is %d\n", a);
  } else if (b >= a && b >= c) {
    printf("The maximum is %d\n", b);
  } else {
    printf("The maximum is %d\n", c);
  }

  return 0;
}
