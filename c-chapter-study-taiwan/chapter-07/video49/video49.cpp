/************************************************************
 * @file: video49.cpp
 * @version: 1.0.0
 *
 * @brief: 对三个变数求最大值（if语句）
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
  int max = a > b ? a : b;
  max = max > c ? max : c;
  printf("The maxinum is %d.\n ", max);
  return 0;
}
