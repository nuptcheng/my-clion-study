# video-59 用排序简化三角形判断条件（使用if语句）

视频47原来代码：
```c++
int main() {
  // 假设必须按照边长大小依次输入，即side1 <= side2 <= side3
  int side1, side2, side3;
  printf("Please enter the lengths: ");
  scanf("%d%d%d", &side1, &side2, &side3);

  // TODO 对输入的三角形三个边长按从小到大顺序排序，见后面视频

  // 正三角形：现在判断逻辑，side2肯定==side1==side3
  if (side1 == side3) {
    printf("Regular triangle\n");
  }

  // 等腰三角形：原现在判断逻辑
  if (side1 == side2 || side2 == side3) {
    printf("Isosceles triangle\n");
  }

  // 直角三角形：现在判断逻辑，长边 = 短边的平方和
  if (pow(side3, 2) == pow(side1, 2) + pow(side2, 2)) {
    printf("Rectangular triangle\n");
  }
  return 0;
}
```



![图片](pics//pic-1.jpg)
