/************************************************************
 * @file: video07.cpp
 * @version: 1.0.0
 *
 * @brief: 16 - 07 ｜ 數字統計的練習 (使用陣列)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <_inttypes.h>
#include <stdio.h>

// 函数声明
int max10(int n[10]);
int avg10(int n[10]);

int main() {
  int i, n[10];
  printf("Enter 10 numbers: ");
  for (i = 1; i <= 10; i++) {
    scanf("%d", &n[i - 1]);
  }

  printf("Max: %d\n", max10(n));
  printf("Avg: %d\n", avg10(n));
  return 0;
}

int max10(int n[10]) {
  int max = n[0];
  for (int i = 1; i < 10; i++) {
    if (n[i] > max) {
      max = n[i];
    }
  }
  return max;
}

int avg10(int n[10]) {
  int sum = n[0];
  for (int i = 1; i < 10; i++) {
    sum += n[i];
  }
  return sum / 10;
}
