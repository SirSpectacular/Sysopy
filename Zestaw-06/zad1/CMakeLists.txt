cmake_minimum_required(VERSION 2.8)
project(IPC C)

add_compile_options(-std=c11)

add_executable(client client.c)
add_executable(server server.c)