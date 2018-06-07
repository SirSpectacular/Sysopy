cmake_minimum_required(VERSION 2.8)
project(Sockets C)


add_compile_options(-std=c11)
add_executable(server server.c)
add_executable(client client.c)
target_link_libraries(client pthread)