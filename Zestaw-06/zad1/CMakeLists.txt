cmake_minimum_required(VERSION 3.10)
project(IPC C)

set(CMAKE_C_STANDARD 11)

add_executable(client client.c)
add_executable(server server.c)