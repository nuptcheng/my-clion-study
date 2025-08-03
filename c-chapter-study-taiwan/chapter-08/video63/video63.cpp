/************************************************************
 * @file: video63.cpp
 * @version: 1.0.0
 *
 * @brief: 简单四则运算练习（if-else语句）
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

  // 运算符号用op代替
  scanf("%d%c%d", &num1, &op, &num2);
  if (op == '+') {
    answer = (double)(num1 + num2);
  } else if (op == '-') {
    answer = (double)(num1 - num2);
  } else if (op == '*') {
    answer = (double)(num1 * num2);
  } else if (op == '/') {
    // 注意这里num1要转成float
    answer = (float)(num1) / num2;
    // answer = static_cast<float>(num1) / num2;
  }

  if (op == '/') {
    printf("Answer is: %f\n", answer);
  } else {
    printf("Answer is: %d\n", (int)answer);
  }
  return 0;
}
