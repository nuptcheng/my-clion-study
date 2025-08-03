/************************************************************
 * @file: video09-02.cpp
 * @version: 1.0.0
 *
 * @brief: 09-02 猜数字练习（while语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int answer = 4;
  int guess;
  // 先执行1次
  printf("Please enter your guess: ");
  scanf("%d", &guess);
  // 有了初始值继续执行
  while (guess != answer) {
    if (guess > answer) {
      printf("Too large\n");
    } else if (guess < answer) {
      printf("Too small\n");
    }
    printf("Please enter your guess: ");
    scanf("%d", &guess);
  }
  printf("Correct\n");
  return 0;
}
