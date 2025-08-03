

交换数字除了用temp临时变量，这样也可以
```c++
    integer1 = integer1 ^ integer2;
    integer2 = integer1 ^ integer2;
    integer1 = integer1 ^ integer2;
```