/************************************************************
 * @file: video28.cpp
 * @version: 1.0.0
 *
 * @brief:
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

int main() {
    // 1
    // printf("size(char) = %d\n", sizeof(char));

    // char ch = 'A';
    // printf("%c\n", ch);

    // B
    char ch = 'A' + 1;
    printf("%c\n", ch);

    // @
    char ch2 = 'A' - 1;
    printf("%c\n", ch2);

    // ASC转换就是65 + 49 = 114，就是r
    char ch3 = 'A' + '1';
    printf("%c\n", ch3);


    return 0;
}
