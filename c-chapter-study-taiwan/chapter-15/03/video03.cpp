/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 15 - 03 ｜ 骰子點數出現次數的統計 (使用陣列)
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
    switch (dice) {
    case 1:
      counter[0]++;
      break;
    case 2:
      counter[1]++;
      break;
    case 3:
      counter[2]++;
      break;
    case 4:
      counter[3]++;
      break;
    case 5:
      counter[4]++;
      break;
    case 6:
      counter[5]++;
      break;
    default:
      break;
    }
  }
  printf("counter1 = %d\n", counter[0]);
  printf("counter2 = %d\n", counter[1]);
  printf("counter3 = %d\n", counter[2]);
  printf("counter4 = %d\n", counter[3]);
  printf("counter5 = %d\n", counter[4]);
  printf("counter6 = %d\n", counter[5]);

  return 0;
}
