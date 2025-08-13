/************************************************************
 * @file: video06.cpp
 * @version: 1.0.0
 *
 * @brief: 13 - 06 ｜ 對三個變數求最大值的練習 (使用函式)

 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>


int max3(int, int, int);
int max2(int, int);

int main() {
  int a, b, c, max;
  printf("Please enter three integers: ");
  scanf("%d%d%d", &a, &b, &c);
  printf("The maxinum is %d. \n", max);
  return 0;
}

int max3(int a, int b, int c) { return max2(max2(a, b), c); }

int max2(int a, int b) {
  if (a >= b) {
    return a;
  } else {
    return b;
  }
}
