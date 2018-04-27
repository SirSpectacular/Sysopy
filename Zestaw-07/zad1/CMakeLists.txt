cmake_minimum_required(VERSION 3.10)
project(IPC_semaphores_sharedmemory C)

set(CMAKE_C_STANDARD 99)

add_executable(barber barber.c)
add_executable(clients clients.c)