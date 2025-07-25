cmake_minimum_required(VERSION 3.16)
project(my_clion_study C CXX)

# 设置 C/C++ 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# 添加子项目
add_subdirectory(c-plus-study)
add_subdirectory(c-study)
add_subdirectory(c-dynamic-compile)
add_subdirectory(c-chapter-study-taiwan)