# 15 - 05 ｜ 隨機存取陣列元素

示例1：优化上面的需求，随机掷骰子6000次，统计1-6出现的次数
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
    // 精华在下面2句，余数是1-6
    int dice = rand() % 6 + 1;
    // 余数是1，counter[0]加一
    counter[dice - 1]++;
  }

  for (int j = 1; j <= 6; ++j) {
    printf("counter%d = %d\n", j, counter[j - 1]);
  }

  return 0;
}
```

示例2需求：
![图片](pics//pic-1.jpg)


示例2实现：
```c++
int main() {
  int total = 0;
  int prices[] = {90, 75, 83, 89, 71};
  int id;
  do {
    printf("Enter id: ");
    scanf("%d", &id);
    // 这要注意prices[-1]是未定义行为
    if (id != 0) {
      total += prices[id - 1];
    }
  } while (id != 0);
  printf("total = %d\n", total);
  return 0;
}
```

上面继续优化如下
```c++
int main() {
  int prices[] = {90, 75, 83, 89, 71};
  int total = 0, id;
  while (1) {
    printf("Enter id: ");
    scanf("%d", &id);
    // 定义跳出while的中断
    if (id == 0) {
      break;
    }
    total += prices[id - 1];
  }
  printf("total = %d\n", total);
  return 0;
}
```