#include <stdio.h>
#include "Windows.h"

void __stdcall printParameters(int i1, int i2, int i3, float f1, float f2, float f3) {
    printf("\t%d; %d; %d\n", i1, i2, i3);
    printf("\t%f; %f; %f\n", f1, f2, f3);
    printf("Sum of int: %d\n", i1 + i2 + i3);
    printf("Sum of float: %f\n", f1 + f2 + f3);
}

extern int __fastcall addParameters(int i1, int i2, int i3, float f1, float f2, float f3);

int main() {
    
    int result = addParameters(1, 20, 300, 12.0, 20.5, 300.125);

    return 0;
}