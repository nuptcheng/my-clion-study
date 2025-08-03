/************************************************************
 * @file: video56.cpp
 * @version: 1.0.0
 *
 * @brief: 对三个变数依照大小排序（使用if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
  int a, b, c, t;
  printf("Please enter three integers: ");
  scanf("%d%d%d", &a, &b, &c);
  printf("Before: %d %d %d\n", a, b, c);

  /**
   * 按从小到大排序的几种情况：
   * a < b < c
   * a < c < b
   * b < a < c
   * b < c < a
   * c < a < b
   * c < b < a
   */

  // TODO 这里不写代码了，视频里面比较啰嗦



  printf("After: %d %d %d\n", a, b, c);
  return 0;
}
