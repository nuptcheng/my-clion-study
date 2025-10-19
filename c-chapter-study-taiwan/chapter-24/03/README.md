# 24 - 02 ｜ 兩個變數數值交換 (使用函式)


使用函数交换数值完整实现：大的数字放后面
```c++
void sort(int *, int *);

void swap(int *, int *);

int main() {
    int a, b;
    printf("Please enter two integers: ");
    scanf("%d%d", &a, &b);
    sort(&a, &b);
    printf("The sorted integers are: \n");
    printf("min: %d\n", a);
    printf("max: %d\n", b);
    return 0;
}

void sort(int *a, int *b) {
    if (*a > *b) {
        swap(a, b);
    }
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}
```


![图片](pics//pic-1.jpg)
