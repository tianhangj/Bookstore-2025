cmake_minimum_required(VERSION 3.10)
project(Bookstore-2025)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
add_executable(code src/main.cpp)