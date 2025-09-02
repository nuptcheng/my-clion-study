/************************************************************
 * @file: video01.cpp
 * @version: 1.0.0
 *
 * @brief: 16 - 01 ｜ 查詢數字的練習 (使用陣列)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int i, n[5];
  for (i = 1; i <= 5; ++i) {
    printf("%d: ", i);
    scanf("%d", &n[i - 1]);
  }

  // 循环输入
  while (1) {
    printf("Q: ");
    scanf("%d", &i);
    // 中断点
    if (i == 0) {
      break;
    }
    printf("%d\n", n[i - 1]);
  }

  return 0;
}
