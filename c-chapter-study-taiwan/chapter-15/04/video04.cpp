/************************************************************
 * @file: video04.cpp
 * @version: 1.0.0
 *
 * @brief: 15 - 04 ｜ 循序存取陣列元素 (使用迴圈)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  // 乱数种子
  srand(time(NULL));
  // 数组的初始化
  int counter[6] = {0};
  int i;
  for (i = 0; i < 6000; ++i) {
    int dice = rand() % 6 + 1;
    for (int j = 1; j <= 6; ++j) {
      if (dice == j) {
        counter[j - 1]++;
      }
    }
  }

  for (int j = 1; j <= 6; ++j) {
    printf("counter%d = %d\n", j, counter[j - 1]);
  }

  return 0;
}
