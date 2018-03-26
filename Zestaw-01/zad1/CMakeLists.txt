cmake_minimum_required(VERSION 3.10)
project(myLibrary C)



set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ")


add_library(myStaticLibrary STATIC library.c)
add_library(mySharedLibrary SHARED library.c)
