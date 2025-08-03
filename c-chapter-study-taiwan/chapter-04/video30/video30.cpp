/************************************************************
 * @file: video30.cpp
 * @version: 1.0.0
 *
 * @brief: 字符大小写转换（大写转换小写）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/

#include <stdio.h>

int main() {
     char input;
    printf("Please enter a character: ");
    scanf("%c", &input);
    const char output = input + 32;
    // const char output = input + ('a' - 'A');
    printf("output: %c\n", output);
    return 0;
}
