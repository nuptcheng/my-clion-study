/************************************************************
 * @file: video06.cpp
 * @version: 1.0.0
 *
 * @brief: 16 - 06 ｜ 求小範圍內眾數的練習 (使用陣列)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  // b[0] ~ b[9]都被初始化为0
  int i, n, b[10] = {0};
  int c[10] = {0, 1};
  int d[10] = {1};
  printf("Enter 10 numbers: ");
  for (i = 1; i <= 10; i++) {
    // 这里n已经初始化了，后面可以使用了，比如输入2次8，那么b[8]=2
    scanf("%d", &n);
    b[n]++;
  }

  int ans = 0;
  for (n = 1; n < 10; n++) {
    // 如果数值一样，显示后面较大的数
    if (b[n] >= b[ans]) {
      ans = n;
    }
  }

  printf("Answer: %d\n", ans);
  return 0;
}
