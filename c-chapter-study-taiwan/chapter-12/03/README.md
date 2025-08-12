# 12 - 03 ｜ 用文字繪製空心長方形的練習 (使用 for 述句)


需求整理：
![图片](pics//pic-1.jpg)


自己的实现方法：
```c++
int main() {
  int N;
  printf("N = ");
  scanf("%d", &N);

  int i, j;
  for (i = 1; i <= N; ++i) {
    for (j = 1; j <= N; ++j) {
      // 第一行和最后一行完整*
      if (i == 1 || i == N) {
        printf("*");
      } else {
        // 每一行头尾*
        if (j == 1 || j == N) {
          printf("*");
        }
        // 每一行其余空白
        else {
          printf(" ");
        }
      }
    }
    printf("\n");
  }

  return 0;
}

```

视频优化的方法：
```c++

```