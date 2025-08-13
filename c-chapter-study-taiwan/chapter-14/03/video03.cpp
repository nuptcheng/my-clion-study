/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 14 - 03 ｜ 求連續整數和的練習 (使用函式)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

// 示例1：自己琢磨版本
// int calc(int);
// int sum = 0;
//
// int main() {
//   int N;
//   printf("enter a number: ");
//   scanf("%d", &N);
//
//   int sum = calc(N);
//   printf("the sum is: %d\n", sum);
//   return 0;
// }
//
// int calc(int i) {
//   if (i >= 1) {
//     sum += i;
//     calc(i - 1);
//   }
//   return sum;
// }

int sum(int);

int main() {
  int N;
  printf("enter a number: ");
  scanf("%d", &N);
  printf("The sum of %d is: %d\n", N, sum(N));
  return 0;
}

int sum(int n) {
  if (n == 1) {
    return 1;
  }
  return n + sum(n - 1);
}
