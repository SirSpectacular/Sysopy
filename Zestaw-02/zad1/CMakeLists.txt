cmake_minimum_required(VERSION 3.10)
project(filesAndDirectories C)

set(CMAKE_C_STANDARD 11)

add_executable(files main.c)
add_executable(clear clear.c)

enable_testing()

add_test(clearing clear)
foreach(loop_var 4 512 4096 8192)
    foreach(loop_var2 3000 5000)

        set(testGenerate "testGenerate${loop_var}x${loop_var2}")
        set(testCopySys  "testCopySys${loop_var}x${loop_var2}")
        set(testCopyLib  "testCopyLib${loop_var}x${loop_var2}")
        set(testSortSys  "testSortSys${loop_var}x${loop_var2}")
        set(testSortLib  "testSortLib${loop_var}x${loop_var2}")

        add_test(${testGenerate} files generate plik1 ${loop_var2} ${loop_var})
        add_test(${testCopySys}  files copy plik1 plik2 ${loop_var2} ${loop_var} sys)
        add_test(${testSortSys}  files sort plik2 ${loop_var2} ${loop_var} sys)
        add_test(${testCopyLib}  files copy plik1 plik2 ${loop_var2} ${loop_var} lib)
        add_test(${testSortLib}  files sort plik2 ${loop_var2} ${loop_var} lib)

    endforeach(loop_var2)
endforeach(loop_var)

