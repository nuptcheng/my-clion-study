/************************************************************
 * @file: video65.cpp
 * @version: 1.0.0
 *
 * @brief: 多选一的路口（switch语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int num1, num2;
  char op;
  double answer;
  printf("Please enter formula: ");
  scanf("%d%c%d", &num1, &op, &num2);
  switch (op) {
  case '+':
    answer = (double)(num1 + num2);
    break;
  case '-':
    answer = (double)(num1 - num2);
    break;
  case '*':
    answer = (double)(num1 * num2);
    break;
  case '/':
    answer = (double)num1 / num2;
    break;
  }

  if (op == '/') {
    printf("The answer is %.2f\n", answer);
  } else {
    printf("The answer is %d\n", (int)(answer));
  }

  return 0;
}
