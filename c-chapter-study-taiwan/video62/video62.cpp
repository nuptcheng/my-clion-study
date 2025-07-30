/************************************************************
 * @file: video62.cpp
 * @version: 1.0.0
 *
 * @brief: 猜数字练习（if-else语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int guess;
  int answer = 4;
  printf("Please enter your guess: ");
  scanf("%d", &guess);

  if (guess > answer) {
    printf("Too large!\n");
  } else if (guess < answer) {
    printf("Too small!\n");
  } else {
    printf("Correct!\n");
  }

  return 0;
}
