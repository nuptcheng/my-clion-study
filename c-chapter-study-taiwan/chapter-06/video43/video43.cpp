/************************************************************
 * @file: video43.cpp
 * @version: 1.0.0
 *
 * @brief: 正三角形判断练习（if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int side1, side2, side3;
  printf("Please enter the lengths: ");
  scanf("%d%d%d", &side1, &side2, &side3);
  if (side1 == side2 && side2 == side3) {
    printf("Regular triangle\n");
  }
  return 0;
}
