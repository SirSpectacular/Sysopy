cmake_minimum_required(VERSION 3.10)
project(myLibrary C)



set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")


add_library(myStaticLibrary STATIC library.c)
add_library(mySharedLibrary SHARED library.c)

add_executable(mainStatic main.c)
add_executable(mainShared main.c)
add_executable(clear clear.c)

target_link_libraries(mainStatic myStaticLibrary)
target_link_libraries(mainShared mySharedLibrary)

enable_testing()

add_test(clearing clear)

add_test(testStatic1 mainStatic 1000 512 dynamic createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testStatic2 mainStatic 1000 512 static addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 3325441 1000)

add_test(testShared1 mainShared 1000 512 static createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testShared2 mainShared 1000 512 dynamic addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)