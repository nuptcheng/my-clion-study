# 15 - 04 ｜ 循序存取陣列元素 (使用迴圈)

数组元素的存取：


示例：修改上一章的实现
```c++
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  // 乱数种子
  srand(time(NULL));
  // 数组的初始化
  int counter[6] = {0};
  int i;
  for (i = 0; i < 6000; ++i) {
    int dice = rand() % 6 + 1;
    for (int j = 1; j <= 6; ++j) {
      if (dice == j) {
        counter[j - 1]++;
      }
    }
  }

  for (int j = 1; j <= 6; ++j) {
    printf("counter%d = %d\n", j, counter[j - 1]);
  }

  return 0;
}
```



![图片](pics//pic-1.jpg)
