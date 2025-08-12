# 12 - 06 ｜ 求簡易整數方程式的練習 (使用 for 述句 )

需求：
- 已知2个正整数相加为30，相乘为221，求此两个正整数


优化后：算法的优化，最小的循环，下面可以看到for循环次数减少
```c++
int main() {
  int i, j;
  for (i = 1; i <= 30 / 2; ++i) {
    int number_1 = i;
    int number_2 = 30 - i;
    if (number_1 * number_2 == 221) {
      printf("%d, %d\n", number_1, number_2);
    }
  }

  return 0;
}
```

![图片](pics//pic-1.jpg)
