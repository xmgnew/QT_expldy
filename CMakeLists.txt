cmake_minimum_required(VERSION 3.16)

project(expldy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 让 VS Code / clangd 能拿到编译参数（补全、跳转更稳）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 兜底（Qt6 下 qt_standard_project_setup 通常会设置，但加上更稳）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

# Qt6 推荐：自动设置一些常用编译选项/警告/平台细节
qt_standard_project_setup()

qt_add_executable(expldy
    main.cpp
    wifelabel.cpp
    wifelabel.h
    audiomanager.h
    audiomanager.cpp
)

# ✅ 关键修复：把 Multimedia 链接进来
target_link_libraries(expldy PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
)

# 可选：对你这种桌宠/2D 小项目很实用
if (WIN32)
    set_target_properties(expldy PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
