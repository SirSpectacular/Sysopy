//
// Created by student on 16.05.18.
//

#include <stdio.h>
#include <sys/stat.h>

int main() {
    remove("./Times.txt");
    mkdir("./res", 0777);
    mkdir("./out", 0777);
}