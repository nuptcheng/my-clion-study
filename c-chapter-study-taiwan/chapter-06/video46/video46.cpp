/************************************************************
 * @file: video46.cpp
 * @version: 1.0.0
 *
 * @brief: 三角形种类判断练习（if语句）
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

  // 正三角形
  if (side1 == side2 && side2 == side3) {
    printf("Regular triangle\n");
  }

  // 等腰三角形
  if (side1 == side2 || side1 == side3 || side2 == side3) {
    printf("Isosceles triangle\n");
  }

  // 直角三角形
  if (pow(side1, 2) == pow(side2, 2) + pow(side3, 2) ||
      pow(side2, 2) == pow(side2, 2) + pow(side1, 2) ||
      pow(side3, 2) == pow(side1, 2) + pow(side2, 2)) {
    printf("Rectangular triangle\n");
  }
  return 0;
}
