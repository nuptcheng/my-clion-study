# 创建动态库 target
add_library(c_dynamic_lib SHARED
        message.c
)

# 创建主可执行程序
add_executable(c_dynamic_exec
        main.c
)

# 设置 include 路径
target_include_directories(c_dynamic_exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 链接可执行文件和动态库
target_link_libraries(c_dynamic_exec PRIVATE c_dynamic_lib)
