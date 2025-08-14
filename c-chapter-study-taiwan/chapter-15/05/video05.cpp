/************************************************************
 * @file: video05.cpp
 * @version: 1.0.0
 *
 * @brief: 15 - 05 ｜ 隨機存取陣列元素
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 示例1
// int main() {
//   // 乱数种子
//   srand(time(NULL));
//   // 数组的初始化
//   int counter[6] = {0};
//   int i;
//   for (i = 0; i < 6000; ++i) {
//     // 精华在下面2句，余数是1-6
//     int dice = rand() % 6 + 1;
//     // 余数是1，counter[0]加一
//     counter[dice - 1]++;
//   }
//
//   for (int j = 1; j <= 6; ++j) {
//     printf("counter%d = %d\n", j, counter[j - 1]);
//   }
//
//   return 0;
// }

// 示例2
int main() {
  int prices[] = {90, 75, 83, 89, 71};
  int total = 0, id;
  while (1) {
    printf("Enter id: ");
    scanf("%d", &id);
    // 定义跳出while的中断
    if (id == 0) {
      break;
    }
    total += prices[id - 1];
  }
  printf("total = %d\n", total);
  return 0;
}
