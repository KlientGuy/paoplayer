cmake_minimum_required(VERSION 3.27)
project(music_widget)

set(CMAKE_CXX_STANDARD 23)

add_executable(music_widget main.cpp
        lib/OP_MASKS.h)
target_include_directories(music_widget PRIVATE ${PROJECT_SOURCE_DIR}/../music_widget_base)
target_link_libraries(music_widget music_widget_base)
add_subdirectory(${PROJECT_SOURCE_DIR}/../music_widget_base build)