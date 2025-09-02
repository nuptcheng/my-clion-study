/************************************************************
 * @file: video02.cpp
 * @version: 1.0.0
 *
 * @brief: 16 - 02 ｜ 查詢範圍內數字的練習 (使用陣列)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int i, n[10];
  printf("Enter 10 integers: ");
  for (i = 1; i <= 10; ++i) {
    scanf("%d", &n[i]);
  }

  while (1) {
    int l, r;
    printf("L R: ");
    scanf("%d%d", &l, &r);
    if (l == 0 && r == 0) {
      break;
    }

    printf("Ans: ");
    for (i = 0; i < 10; ++i) {
      if (n[i] >= l && n[i] <= r) {
        printf("%d ", n[i]);
      }
    }
    printf("\n");
  }

  return 0;
}
