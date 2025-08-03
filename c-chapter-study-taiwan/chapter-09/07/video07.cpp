/************************************************************
 * @file: video09-07.cpp
 * @version: 1.0.0
 *
 * @brief: 09-07至少做一次的重复执行（do-while语句）
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
  do {
    printf("Please enter your guess: ");
    scanf("%d", &guess);
    if (guess > answer) {
      printf("Too large！\n");
    } else if (guess < answer) {
      printf("Too small\n");
    } else {
      printf("Correct！");
    }
  } while (guess != answer);
  return 0;
}
