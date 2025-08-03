/************************************************************
 * @file: video50.cpp
 * @version: 1.0.0
 *
 * @brief: 对四个变数求最大值（if语句）
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

  int temp1 = a > b ? a : b;
  int temp2 = c > d ? c : d;
  int max = temp1 > temp2 ? temp1 : temp2;
  printf("The maxinum is %d.\n ", max);
  return 0;
}
