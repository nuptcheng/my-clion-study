/************************************************************
 * @file: video04.cpp
 * @version: 1.0.0
 *
 * @brief: 16 - 04 ｜ 查詢最接近數字的練習 (使用陣列)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>

int main() {
  int q, n[10];
  printf("Enter 10 integers: ");
  for (int i = 0; i < 10; i++) {
    scanf("%d", &n[i]);
  }
  while (1) {
    printf("Q: ");
    scanf("%d", &q);
    if (q == 0) {
      break;
    }
    // 先把最接近的数赋予数组第1个元素
    int nearest_n = n[0];
    // 最接近的数和目标值的差值绝对值（先用n[0]计算，然后不断缩小范围）
    int nearest_d = abs(q - n[0]);
    for (int i = 1; i < 10; i++) {
      int d = abs(q - n[i]);
      if (d < nearest_d || (d == nearest_n && n[i] < nearest_n)) {
        nearest_d = d;
        nearest_n = n[i];
      }
    }
    printf("%d\n", nearest_n);
  }

  return 0;
}
