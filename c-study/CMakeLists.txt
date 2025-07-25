# C子模块
add_executable(c_demo
        message.c
        main.c
        ../test/test.cpp
        ../test/test.h
)

# 你可以添加 include 目录
target_include_directories(c_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
