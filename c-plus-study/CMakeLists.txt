# C++子模块
add_executable(cpp_demo
        main.cpp
        message.cpp
)

# 你可以添加 include 目录
target_include_directories(cpp_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
