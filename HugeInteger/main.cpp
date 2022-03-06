#include <iostream>


#include "huge_integer.h"



int main()
{
    HugeInt a(-(1 << 31) + (1 << 30) + 1);
    HugeInt b(-(1 << 31) + (1 << 29) + (1 << 28) + 3);

    HugeInt c = a + b;
}