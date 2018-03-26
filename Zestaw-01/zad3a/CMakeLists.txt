cmake_minimum_required(VERSION 3.10)
project(myLibrary C)



set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")


add_library(myStaticLibrary STATIC library.c)
add_library(mySharedLibrary SHARED library.c)
add_library(myDynamicLibrary MODULE library.c)

add_executable(mainStatic main.c)
add_executable(mainShared main.c)
add_executable(mainDynamic main_dynamic.c)
add_executable(clear clear.c)

target_link_libraries(mainStatic myStaticLibrary)
target_link_libraries(mainShared mySharedLibrary)
target_link_libraries(mainDynamic ${CMAKE_DL_LIBS})

enable_testing()

add_test(clearing clear)

add_test(test mainStatic)

add_test(testStatic1s mainStatic 1000 512 static createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testStatic2s mainStatic 1000 512 static addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 3325441 1000)
add_test(testStatic1d mainStatic 1000 512 dynamic createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testStatic2d mainStatic 1000 512 dynamic addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 3325441 1000)

add_test(testShared1s mainShared 1000 512 static createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testShared2s mainShared 1000 512 static addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)
add_test(testShared1d mainShared 1000 512 dynamic createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testShared2d mainShared 1000 512 dynamic addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)

add_test(testDynamic1s mainDynamic 1000 512 static createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testDynamic2s mainDynamic 1000 512 static addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)
add_test(testDynamic1d mainDynamic 1000 512 dynamic createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
add_test(testDynamic2d mainDynamic 1000 512 dynamic addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)


