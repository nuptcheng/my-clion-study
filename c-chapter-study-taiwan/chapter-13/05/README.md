# 13 - 05 ｜ 關於變數名稱可視範圍 (scope)

变量名称的声明可以分成3类：
- 全局变量：在函数定义外，容易造成全局污染

    ```c++
    int i;
    int main() {
    
        return 0;
    }
    ```

- 局域变量：函数内定义
    ```c++
    int main() {
        int i;
        return 0;
    }
    ```
  
- 形式参数：函数的参数里面定义
  ```c++
  int f(int i) {
  return 0;
  }
    ```
  
在同一组 `{}` 里同名变量只能有1个，如下是2个i，可以不报错
```c++
// 合法
int i;

int main() {
// 合法
  int i;
  {
  // 合法
    int i;
  }
  return 0;
}
```

- 函数的形式参数比较特殊，它的范围是函数的 `{}`，比如下面是报错的
    ```c++
  int f(int i) {
  // 报错
  int i;
  return 0;
  }
  ```

示例1：注意范围小的优先！
```c++
int i = 1;

int main() {
  printf("%d\n", i); // 1
  int i = 2;
  printf("%d\n", i); // 2

  {
    printf("%d\n", i); // 2
    int i = 3;
    printf("%d\n", i); // 3
  }
  printf("%d\n", i); // 2
  return 0;
}
```

![图片](pics//pic-1.jpg)


示例2：
```c++
int main() {
  int i = 3;
  printf("%d\n", i); // 3

  if (i == 3) {
    i = i + 1;
    int i = 6;
    printf("%d\n", i); // 6
    i = i + 1;
  }
  if (i == 3) { // DEBUG时这时i为4，所以不相等
    // 不输出
    printf("%d\n", i);
  }
  return 0;
}
```