cmake_minimum_required(VERSION 3.27)
project(music_widget_base)

set(CMAKE_CXX_STANDARD 23)

add_library(music_widget_base STATIC PaConnector.cpp
        SongController.cpp
        SongController.h
        ConfigParser.cpp
        ConfigParser.h
        paop_config.h
        Lexer.h)
target_include_directories(music_widget PRIVATE /usr/include/pulse)
target_link_libraries(music_widget pulse)
