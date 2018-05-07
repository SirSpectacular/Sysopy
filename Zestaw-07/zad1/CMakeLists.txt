cmake_minimum_required(VERSION 2.8)
project(IPC_semaphores_sharedmemory C)

add_compile_options(-std=gnu11)

add_executable(barber barber.c)
add_executable(clients clients.c)