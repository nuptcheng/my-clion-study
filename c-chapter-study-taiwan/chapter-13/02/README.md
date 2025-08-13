# 13 - 02 ｜ 更多關於函式使用的細節

函数声明：
- 函数必须在使用前声明！
- 函数声明的参数名可以省略，只放类型

![图片](pics//pic-1.jpg)

```c++
// 函数声明，可以省略参数
int f(int);

int main() {
  printf("%d\n", f(3));
  return 0;
}


int f(int x) { return x + 3; }
```