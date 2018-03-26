cmake_minimum_required(VERSION 2.8)
project(processes C)

add_compile_options(-std=c99)

add_executable(processes main.c )
add_executable(testMem testMem.c)
add_executable(testTime testTime.c)