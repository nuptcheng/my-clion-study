/************************************************************
 * @file: video51.cpp
 * @version: 1.0.0
 *
 * @brief: 对多个变数求最小值（使用if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
  int a, b, c, d;
  printf("Please enter four integers: ");
  scanf("%d%d%d%d", &a, &b, &c, &d);

  int temp1 = a < b ? a : b;
  int temp2 = c < d ? c : d;
  int min = temp1 < temp2 ? temp1 : temp2;
  printf("The minium is %d.\n ", min);
  return 0;
}
