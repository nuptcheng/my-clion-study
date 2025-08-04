/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 10 - 03 ｜ 數等差數列的練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {

  // 从10打印到1
  // int count;
  // for (count = 1; count <= 10; count++) {
  //   int number = 11 - count;
  //   printf("%d\n", number);
  // }

  // 打印出1到10之间的奇数：方法1
  // int count;
  // for (count = 1; count <= 5; count++) {
  //   int number = 2 * count - 1;
  //   printf("%d\n", number);
  // }

  // 打印出1到10之间的奇数：方法2
  int count;
  for (count = 1; count <= 10; count++) {
    // 判断count是奇数
    if (count % 2 == 1) {
      printf("%d\n", count);
    }
  }

  return 0;
}
