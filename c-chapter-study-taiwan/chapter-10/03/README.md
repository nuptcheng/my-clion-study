# 10-03 数等差数列的练习（for语句）

从10打印到1
```c++

  // 从10打印到1
  int count;
  for (count = 1; count <= 10; count++) {
    int number = 11 - count;
    printf("%d\n", number);
  }
```

打印出1到10之间的奇数：方法1
```c++
  // 打印出1到10之间的奇数
  int count;
  for (count = 1; count <= 5; count++) {
    int number = 2 * count - 1;
    printf("%d\n", number);
  }
```

打印出1到10之间的奇数：方法2
```c++
  // 打印出1到10之间的奇数：方法2
  int count;
  for (count = 1; count <= 10; count++) {
    // 判断count是奇数
    if (count % 2 == 1) {
      printf("%d\n", count);
    }
  }
```


![图片](pics//pic-1.jpg)
