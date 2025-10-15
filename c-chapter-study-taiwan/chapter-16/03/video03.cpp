/************************************************************
 * @file: video03.cpp
 * @version: 1.0.0
 *
 * @brief: 16 - 03 ｜ 查詢上限內最大數字的練習 (使用陣列)
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

// int main() {
//   int n[10], q, max = -1;
//   printf("Enter 10 integers: ");
//   for (int i = 0; i < 10; ++i) {
//     scanf("%d", &n[i]);
//   }
//   printf("Q: ");
//   scanf("%d", &q);
//   for (int i = 0; i < 10; ++i) {
//     if (n[i] <= q && n[i] > max) {
//       max = n[i];
//     }
//   }
//   if (max != -1) {
//     printf("%d\n", max);
//   }
//   return 0;
// }

int main() {
  int n[10], q;
  printf("Enter 10 integers: ");
  for (int i = 0; i < 10; ++i) {
    scanf("%d", &n[i]);
  }
  while (1) {
    // index
    int max_i = -1;
    printf("Q: ");
    scanf("%d", &q);
    if (q == 0) {
      break;
    }

    for (int i = 0; i < 10; ++i) {
      // max_i为-1，就不会计算n[max_i]
      if (n[i] <= q && (max_i == -1 || n[i] > n[max_i])) {
        max_i = i;
      }
    }
    if (max_i != -1) {
      printf("%d\n", n[max_i]);
    }
  }

  return 0;
}
