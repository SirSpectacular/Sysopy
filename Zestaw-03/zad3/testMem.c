//
// Created by student on 24.03.18.
//

#include <malloc.h>

#define N 10000000

int main() {
    char * constructor;
    while (1) {
    constructor = malloc(N);
    for(int i = 0; i < N; i++)
        constructor[i] = '\0';
}
    return 0;
}