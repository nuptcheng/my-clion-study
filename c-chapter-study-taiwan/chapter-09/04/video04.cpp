/************************************************************
 * @file: video09-04.cpp
 * @version: 1.0.0
 *
 * @brief: 09-04 求不定个数正整数和（while语句）
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
  int count = 0;
  printf("Please enter the numbers（0 quit）: \n");
  while (count == 0 || number != 0) {
    scanf("%d", &number);
    sum += number;
    count += 1;
  }
  printf("The sum is: %d\n", sum);
  return 0;
}
