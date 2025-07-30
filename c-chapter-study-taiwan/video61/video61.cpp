/************************************************************
 * @file: video61.cpp
 * @version: 1.0.0
 *
 * @brief: 两个变数比大小（if-else语句）
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
  if (a > b) {
    printf("a > b\n");
  } else if (a < b) {
    printf("a < b\n");
  } else {
    printf("a = b\n");
  }

  return 0;
}
