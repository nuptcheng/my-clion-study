# 09-03 数数猜了几次的练习（while语句）

需求变更：
- 不仅显示使用者猜数字猜的不不对，也显示使用者猜了几次

原来实现：
```c++
int main() {

  int answer = 4;
  int guess;
  int count = 0;
  // 先执行1次
  printf("Please enter your guess: ");
  scanf("%d", &guess);
  // 第1次猜测也算1次
  count += 1;
  // 有了初始值继续执行
  while (guess != answer) {
    if (guess > answer) {
      printf("Too large\n");
    } else if (guess < answer) {
      printf("Too small\n");
    }
    printf("Please enter your guess: ");
    scanf("%d", &guess);
    count += 1;
  }
  printf("Correct! (%d)\n", count);
  return 0;
}
```

现在引入count作为while判断条件，可以简化如下：
```c++

```


![图片](pics//pic-1.jpg)
