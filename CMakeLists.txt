cmake_minimum_required(VERSION 3.16)

project(expldy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 让 VS Code / clangd 能拿到编译参数（补全、跳转更稳）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

# Qt6 推荐：自动设置一些常用编译选项/警告/平台细节
qt_standard_project_setup()

qt_add_executable(expldy
    main.cpp
    wifelabel.cpp
    wifelabel.h
)

target_link_libraries(expldy PRIVATE Qt6::Widgets)

# 可选：对你这种桌宠/2D 小项目很实用
# - 关闭控制台窗口：Windows 下运行时不弹黑框（需要 GUI 子系统）
if (WIN32)
    set_target_properties(expldy PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
