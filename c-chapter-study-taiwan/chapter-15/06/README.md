# 15 - 06 ｜ 兩顆骰子點數和出現次數統計的練習 (使用陣列)

需求：
![图片](pics//pic-1.jpg)

需求分析：
- 两颗骰子最小是2，最大是12，所以是11个变数，数组为11

代码实现：
```c++
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  // 乱数种子
  srand(time(NULL));
  // 两颗骰子最小是2，最大是12，所以是11个变数，数组为11
  int counter[11] = {0};
  int i;
  for (i = 1; i <= 10000; ++i) {
    int dice1 = rand() % 6 + 1;
    int dice2 = rand() % 6 + 1;
    int sum = dice1 + dice2;
    // 数组元素0对应2，数组元素10对应最大的12（2个6点）
    counter[sum - 2]++;
  }

  for (int i = 2; i <= 12; i++) {
    printf("%d: %d\n", i, counter[i - 2]);
  }
  return 0;
}
```


同上，打印数组长度需要用到`sizeof()`方法
```c++
  // 打印数组长度
  int size_t = sizeof(counter) / sizeof(counter[0]);
  printf("counter length: %d\n", size_t);
```