/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 11 - 03 ｜ 有範圍的韓信點兵問題 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int min, max;
  printf("MIN =  ");
  scanf("%d", &min);
  printf("MAX =  ");
  scanf("%d", &max);
  for (int i = min; i <= max; ++i) {
    if (i % 3 == 2 && i % 5 == 3 && i % 7 == 2) {
      printf("%d\n", i);
    }
  }

  return 0;
}
