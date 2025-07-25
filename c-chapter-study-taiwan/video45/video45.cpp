/************************************************************
 * @file: video45.cpp
 * @version: 1.0.0
 *
 * @brief: 直角三角形判断练习（if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <math.h>
#include <stdio.h>

int main() {
  int side1, side2, side3;
  printf("Please enter the lengths: ");
  scanf("%d%d%d", &side1, &side2, &side3);
  // pow()返回浮点数
  if (pow(side1, 2) == pow(side2, 2) + pow(side3, 2) ||
      pow(side2, 2) == pow(side2, 2) + pow(side1, 2) ||
      pow(side3, 2) == pow(side1, 2) + pow(side2, 2)) {
    printf("Rectangular triangle\n");
  }
  return 0;
}
