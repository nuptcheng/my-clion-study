/************************************************************
 * @file: video60.cpp
 * @version: 1.0.0
 *
 * @brief: 二选一的交叉路口（if-else语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int grade;
  printf("Please enter the grade: ");
  scanf("%d", &grade);
  if (grade >= 60) {
    printf("PASS");
  } else {
    printf("FAIL");
  }
  return 0;
}
