/************************************************************
 * @file: video09-05.cpp
 * @version: 1.0.0
 *
 * @brief: 09-05 求不定个数正整数平均（while语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int number;
  int sum = 0;
  float average;
  int count = 0;
  // 先做1次
  printf("Please enter the numbers（0 quit）: \n");
  scanf("%d", &number);
  count += 1;

  while (number != 0) {
    scanf("%d", &number);
    sum += number;
    average = (float)sum / count;
    count += 1;
  }
  printf("total is：%d\n",count);
  printf("The average is: %0.2f\n", average);
  return 0;
}
