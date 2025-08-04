/************************************************************
 * @file: video07.cpp
 * @version: 1.0.0
 *
 * @brief: 10 - 07 ｜ 中止重複執行 (break 與 continue 述句)

 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int max;
  printf("Enter max number: ");
  scanf("%d", &max);

  int number;
  for (number = max; number >= 1; --number) {
    // 韩信点兵问题
    if (number % 3 == 2 && number % 5 == 3 && number % 7 == 2) {
      // 找到了
      printf("number = %d\n", number);
      // 结束for循环和while循环
      break;
    }
  }

  return 0;
}
