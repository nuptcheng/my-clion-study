/************************************************************
 * @file: video05.cpp
 * @version: 1.0.0
 *
 * @brief: 13 - 05 ｜ 關於變數名稱可視範圍 (scope)

 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

// int i = 1;

// int main() {
//   printf("%d\n", i); // 1
//   int i = 2;
//   printf("%d\n", i); // 2
//
//   {
//     printf("%d\n", i); // 2
//     int i = 3;
//     printf("%d\n", i); // 3
//   }
//   printf("%d\n", i); // 2
//   return 0;
// }

// int f(int i) {
//   // 报错
//   int i;
//   return 0;
// }

int main() {
  int i = 3;
  printf("%d\n", i); // 3

  if (i == 3) {
    i = i + 1;
    int i = 6;
    printf("%d\n", i); // 6
    i = i + 1;
  }
  if (i == 3) { // DEBUG时这时i为4，所以不相等
    // 不输出
    printf("%d\n", i);
  }
  return 0;
}