cmake_minimum_required(VERSION 2.8)
project(Threads_basics C)

add_compile_options(-std=gnu11)
SET(GCC_COVERAGE_LINK_FLAGS    "-pthread -lm")
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )
add_executable(Threads_basics main.c)