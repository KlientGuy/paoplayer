cmake_minimum_required(VERSION 3.27)
project(pao_player_base)

set(CMAKE_CXX_STANDARD 23)

add_library(pao_player_base STATIC PaConnector.cpp
        SongController.cpp
        SongController.h
        ConfigParser.cpp
        ConfigParser.h
        paop_config.h
        Lexer.h)
target_include_directories(paoplayer PRIVATE /usr/include/pulse)
target_link_libraries(paoplayer pulse)
