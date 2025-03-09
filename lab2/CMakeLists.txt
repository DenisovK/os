cmake_minimum_required(VERSION 3.0)
project(MatrixMultiplication C)

set(CMAKE_C_STANDARD 99)

add_executable(MatrixMultiplication src/main.c)
target_link_libraries(MatrixMultiplication ws2_32) 
