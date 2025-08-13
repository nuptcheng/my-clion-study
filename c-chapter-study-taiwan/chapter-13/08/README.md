# 13 - 08 ｜ 亂數生成 (使用函式)（重点）


stdio.h 和 stdlib.h 都属于 C 标准库（C Standard Library）头文件，它们并不是需要额外 apt install 的包，而是 编译器（比如 gcc、clang）自带的。

生成随机数：
```c++
int main() {
  // srand(2);
  srand(time(0));
  int i;
  for (i = 0; i < 5; ++i) {
    printf("number = %d\n", rand());
  }
  return 0;
}
```

chatgpt解答：
1. rand()
   作用：生成一个伪随机数（pseudo-random number），范围是 0 到 RAND_MAX（RAND_MAX 通常是一个较大的整数，比如 2147483647）。

特点：是可预测的，如果种子相同，每次运行程序生成的随机数序列是一样的。

2. srand()
   作用：设置随机数生成器的种子（seed）。

语法：
```c++
void srand(unsigned int seed);

```
如果你用相同的 seed 调用 srand()，后续 rand() 产生的随机数序列完全相同。

如果你不调用 srand()，程序默认的种子是 1，所以每次运行随机数序列都一样。

3. time(0)（或 time(NULL)）
   作用：获取当前的时间（Unix 时间戳），单位是秒，从 1970-01-01 00:00:00 UTC 到现在的秒数。

time(0) 等价于 time(NULL)，返回一个 time_t 类型的整数。

例子：

```c++
printf("%ld\n", time(0));  // 比如输出 1734141802

```

4. srand(time(0));
   作用：用当前时间（秒级）作为随机数种子，让每次运行程序时的随机数序列不一样。

原理：因为时间每秒都变化，所以种子不同，rand() 生成的序列也不同。

如果你换成 srand(2);，每次运行都会得到相同的 5 个数。

总结：
rand()：生成伪随机数

srand(seed)：设置伪随机数序列的起点（种子）

time(0)：返回当前时间的秒数

srand(time(0))：用当前时间作为种子，让随机数序列每次运行都不同



![图片](pics//pic-1.jpg)
