/************************************************************
 * @file: video02.cpp
 * @version: 1.0.0
 *
 * @brief: 13 - 02 ｜ 更多關於函式使用的細節
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>
double div(double);

int main() {
  int N;
  printf("Enter a number: ");
  scanf("%d", &N);
  printf("%f\n", div(N * 2));
  return 0;
}

double div(double x) { return x / 2; }
