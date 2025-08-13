/************************************************************
 * @file: video09.cpp
 * @version: 1.0.0
 *
 * @brief: 13 - 09 ｜ 擲骰子的練習 (使用函式)

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
  // 设置乱数种子
  srand(time(NULL));
  int i;

  // 生成1到6之间随机数
  for (i = 1; i <= 5; ++i) {
    int dice = rand() % 6 + 1;
    printf("%d\n", dice);
  }

  return 0;
}
