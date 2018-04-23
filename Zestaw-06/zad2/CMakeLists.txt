cmake_minimum_required(VERSION 2.8)
project(IPC C)

add_compile_options(-std=c11)

SET(GCC_COVERAGE_LINK_FLAGS    "-lrt")
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

add_executable(client client.c)
add_executable(server server.c)