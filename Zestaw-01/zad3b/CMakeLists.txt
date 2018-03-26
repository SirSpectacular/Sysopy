cmake_minimum_required(VERSION 3.10)
project(myLibrary C)



set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")

add_executable(clear clear.c)

foreach(loop_var -O -O1 -O2 -O3 -Os)
    set(sharedName "mySharedLibrary${loop_var}")
    set(staticName "myStaticLibrary${loop_var}")
    set(dynamicName "myDynamicLibrary${loop_var}")

    add_library (${sharedName} SHARED library.c)
    add_library (${staticName} STATIC library.c)
    add_library (${dynamicName} MODULE library.c)

    target_compile_options(${sharedName} PUBLIC ${loop_var})
    target_compile_options(${staticName} PUBLIC ${loop_var})
    target_compile_options(${dynamicName} PUBLIC ${loop_var})

    set(mainShared "mainShared${loop_var}")
    set(mainStatic "mainStatic${loop_var}")
    set(mainDynamic "mainDynamic${loop_var}")

    add_executable(${mainStatic} main.c)
    add_executable(${mainShared} main.c)
    add_executable(${mainDynamic} main_dynamic.c)

    target_compile_options(${mainStatic} PUBLIC ${loop_var})
    target_compile_options(${mainShared} PUBLIC ${loop_var})
    target_compile_options(${mainDynamic} PUBLIC ${loop_var})

    target_link_libraries(${mainStatic} ${staticName})
    target_link_libraries(${mainShared} ${sharedName})
    target_link_libraries(${mainDynamic} ${CMAKE_DL_LIBS})
endforeach(loop_var)

enable_testing()

add_test(testClear clear)

foreach(loop_var -O -O1 -O2 -O3 -Os)
    set(mainStatic "mainStatic${loop_var}")
    set(mainShared "mainShared${loop_var}")
    set(mainDynamic "mainDynamic${loop_var}")

    set(testStatic1s "testStatic1s${loop_var}")
    set(testStatic2s "testStatic2s${loop_var}")
    set(testStatic1d "testStatic1d${loop_var}")
    set(testStatic2d "testStatic2d${loop_var}")

    set(testShared1s "testShared1s${loop_var}")
    set(testShared2s "testShared2s${loop_var}")
    set(testShared1d "testShared1d${loop_var}")
    set(testShared2d "testShared2d${loop_var}")

    set(testDynamic1s "testDynamic1s${loop_var}")
    set(testDynamic2s "testDynamic2s${loop_var}")
    set(testDynamic1d "testDynamic1d${loop_var}")
    set(testDynamic2d "testDynamic2d${loop_var}")

    add_test(${testStatic1s} ${mainStatic} 1000 512 static createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
    add_test(${testStatic2s} ${mainStatic} 1000 512 static addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 3325441 1000)
    add_test(${testStatic1d} ${mainStatic} 1000 512 dynamic createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
    add_test(${testStatic2d} ${mainStatic} 1000 512 dynamic addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 3325441 1000)

    add_test(${testShared1s} ${mainShared} 1000 512 static createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
    add_test(${testShared2s} ${mainShared} 1000 512 static addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)
    add_test(${testShared1d} ${mainShared} 1000 512 dynamic createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
    add_test(${testShared2d} ${mainShared} 1000 512 dynamic addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)

    add_test(${testDynamic1s} ${mainDynamic} 1000 512 static createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
    add_test(${testDynamic2s} ${mainDynamic} 1000 512 static addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)
    add_test(${testDynamic1d} ${mainDynamic} 1000 512 dynamic createAndFreeArays 100 addBlocks 1000 removeBlocks 1000)
    add_test(${testDynamic2d} ${mainDynamic} 1000 512 dynamic addAndRemoveBlocksP 1000 addAndRemoveBlocksR 1000 searchElement 1023244 1000)
endforeach(loop_var)