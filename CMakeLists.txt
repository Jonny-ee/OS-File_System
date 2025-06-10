cmake_minimum_required(VERSION 3.29)
project(Project3)

set(CMAKE_CXX_STANDARD 20)

add_executable(Project3 main.cpp
        file.h
        file_manager.h
        file.cpp
        file_manager.cpp
        file_terminal.cpp
        file_terminal.h
        json.hpp
)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

target_link_libraries(Project3 ${CURSES_LIBRARIES})
