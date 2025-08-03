/************************************************************
 * @file: video09-03.cpp
 * @version: 1.0.0
 *
 * @brief: 09-03 数数猜了几次的练习（while语句）
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
  int count = 0;
  // 引入count == 0 保证while第1次一定会执行
  while (count == 0 || guess != answer) {
    printf("Please enter your guess: ");
    scanf("%d", &guess);
    if (guess > answer) {
      printf("Too large\n");
    } else if (guess < answer) {
      printf("Too small\n");
    }
    count += 1;
  }
  printf("Correct! (%d)\n", count);
  return 0;
}
