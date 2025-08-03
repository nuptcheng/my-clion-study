/************************************************************
 * @file: video09-01.cpp
 * @version: 1.0.0
 *
 * @brief: 09-01 有条件的重复执行（while语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int count = 0;
  while (count < 10) {
    printf("%d\n", count);
    count += 1;
  }
  printf("final count: %d\n", count);
  return 0;
}
