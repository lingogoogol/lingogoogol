#include <stdio.h>
#include <stdlib.h>
#include <iostream>

int main(){
    int a = 10;
    int b = -16;
    int c = -40;
    int d = a%4;
    int sum = 0;
    sum += b*8 + a/8 + (b/4 + 66 - 30)*9 + a*3.5 + abs(c) - d;
    std::cout << sum;
    return 0;
}



