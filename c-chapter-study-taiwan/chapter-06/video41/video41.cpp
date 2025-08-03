/************************************************************
 * @file: video41.cpp
 * @version: 1.0.0
 *
 * @brief: 猜数字（if语句）
 *       [1]
 *       [2]
 *       [3]
 *
 * @author: BecomeBamboo
 * @lastEditTime: 2025/7/22 10:37
 ************************************************************/


#include <stdio.h>

// int main() {
//     int a = 5;
//     if (a > 4) {
//         printf("a is larger than 4.\n");
//     }
//     if (a < 4) {
//         printf("a is smaller than 4.\n");
//     }
//     // 这里是故意搞错，非==，已经把a赋值了4
//     if (a = 4) {
//         printf("a is equal to 4.\n");
//     }
//     return 0;
// }

/**
 * 猜数字
 *
 * @return
 */
int main() {
    int answer = 4;
    int guess;
    printf("Please enter your gusee: ");
    scanf("%d", &guess);
    if (guess > answer) {
        printf("Too large!\n");
    }
    if (guess < answer) {
        printf("Too small!\n");
    }
    if (guess == answer) {
        printf("Correct, You win!\n");
    }
    return 0;
}
