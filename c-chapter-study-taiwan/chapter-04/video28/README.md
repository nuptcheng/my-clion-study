# 字符char类型

## 视频28 char类型概念

台湾话元组就是字节Byte，位元是Bit
```c++
int main() {
    // 1
    printf("size(char) = %d\n", sizeof(char));
    return 0;
}
```

char类型特色
- 常见的是使用ASCII编码
- char类型占用1个Byte
- char类型是一个int类型

单引号包住的是char，比如 `'A'`，双引号包住的是字符串，比如 `"Hello World"`

char类型的printf和scanf
- 使用 `%c`

## 视频29 char类型做运算

'A' + 1，输出为'B'，因为'A'在ASCII是65，那么66就是'B'

char类型计算就是对整数进行计算
```c++
    // @
    char ch2 = 'A' - 1;
    printf("%c\n", ch2);
    
    // ASC转换就是65 + 49 = 114，就是r
    char ch3 = 'A' + '1';
    printf("%c\n", ch3);
```