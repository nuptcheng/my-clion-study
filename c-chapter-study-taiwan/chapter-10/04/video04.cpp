/************************************************************
 * @file: video04.cpp
 * @version: 1.0.0
 *
 * @brief: 10 - 04 ｜ 數特定條件數列的練習 (使用 for 述句)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  // 示例1：打印出1到10之间的偶数
  // int count;
  // for (count = 1; count <= 10; count++) {
  //   if (count % 2 == 0) {
  //     printf("%d\n", count);
  //   }
  // }

  // 示例2：打印出1到10之间不是3的倍数的偶数
  int count;
  for (count = 1; count <= 10; count++) {
    if (count % 2 == 0 && count % 3 != 0) {
      printf("%d\n", count);
    }
  }





  return 0;
}
