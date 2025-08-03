/************************************************************
 * @file: video57.cpp
 * @version: 1.0.0
 *
 * @brief: 用两个变数的数值交换对三个变数做排序上（使用if语句）
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
  * - 将a,b,c中最小的数值换到a
        - 将a,b中较小的数值换到a
        - 将a,c中较小的数值换到a
    - 将b,c中较小的数值换到b
  */

  // 先判断b是否小于a ==>
  // 更新后的b是原来a,b中的较大值，更新后的a是原来a,b中的较小值
  if (b < a) {
    t = b;
    b = a;
    a = t;
  }

  // 再判断c是否小于a ==>
  // 更新后的c是原来c,a中的较大值，更新后的a是原来a,c中的较小值
  if (c < a) {
    t = c;
    c = a;
    a = t;
  }

  // 最后判断c和b ==> 更新后的c是上面更新后的a,b中的较大值 ==>
  // 最后a,b,c就是按从小往大排序
  if (c < b) {
    t = c;
    c = b;
    b = t;
  }

  printf("After: %d %d %d\n", a, b, c);
  return 0;
}
