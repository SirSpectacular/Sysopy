cmake_minimum_required(VERSION 2.8)
project(pipes C)

add_compile_options(-std=c11)

add_executable(master master.c)
add_executable(slave slave.c)