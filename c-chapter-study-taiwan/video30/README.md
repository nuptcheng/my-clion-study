# 字符大小写转换

```c++
    // 方法1
    const char output = input + 32;
    
    // 方法2，不知道大写到小写是多少的时候
    const char output = input + ('a' - 'A');
```

完整代码见viedo30.cpp