/************************************************************
 * @file: video09-09.cpp
 * @version: 1.0.0
 *
 * @brief: 09-09消费金额计算的练习（switch语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int total = 0;
  int id;
  do {
    printf("Please enter your id: ");
    scanf("%d", &id);
    switch (id) {
    case 1:
      total += 90;
      break;
    case 2:
      total += 75;
      break;
    case 3:
      total += 83;
      break;
    case 4:
      total += 89;
      break;
    case 5:
      total += 71;
      break;
    default:
      break;
    }
  } while (id != 0);

  printf("total is : %d\n", total);
  return 0;
}
