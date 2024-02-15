cmake_minimum_required(VERSION 3.27)
project(paoplayer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "-std=c++2b")
set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION "-std=gnu++2b")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(debug)
    message("Compiling with debug true")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

add_executable(paoplayer main.cpp lib/OP_MASKS.h)

target_include_directories(paoplayer PRIVATE ${PROJECT_SOURCE_DIR}/../pao_player_base)

target_link_libraries(paoplayer pao_player_base)
add_subdirectory(${PROJECT_SOURCE_DIR}/../pao_player_base build)
