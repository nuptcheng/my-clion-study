/************************************************************
 * @file: video66.cpp
 * @version: 1.0.0
 *
 * @brief: ID查询练习（switch语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int id;
  printf("Enter id: ");
  scanf("%d", &id);
  switch (id) {
  case 1:
    printf("John\n");
    break;
  case 2:
    printf("Bob\n");
    break;
  case 3:
    printf("London\n");
    break;
  default:
    printf("Wrong answer\n");
    break;
  }

  return 0;
}
