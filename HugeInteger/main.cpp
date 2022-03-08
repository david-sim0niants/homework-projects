#include <iostream>


#include "huge_integer.h"



int main()
{
    HugeInt a((1 << 31) + (1 << 30) + 1);
    HugeInt b((1 << 30) + (1 << 29) + (1 << 28) + 3);

    HugeInt c = a + b;

    HugeInt n(12), k(15);
    HugeInt m = n * k;

    std::cout << n << " * " << k << " = " << m << '\n';
    
    m = a * b;
    std::cout << a << " * " << b << " = " << m << '\n';

    std::cout << a << " * " << m << " = " << a * m << '\n';


    HugeInt p = (std::string)"213575798127510924769817501925972180512";
    HugeInt new_p = p;
    HugeInt::BaseUint t = (1 << 29) + (1 << 30) + 1;
    HugeInt::multiply_by_BaseUint(new_p, t);
    std::cout << p << " * " << t << " = " << new_p << '\n';

    new_p = p;
    HugeInt::sum(new_p, t);
    std::cout << p << " + " << t << " = " << new_p << '\n';


    std::string x_s, y_s;
    while (true)
    {
        std::cin >> x_s >> y_s;
        HugeInt x = x_s;
        HugeInt y = y_s;
        std::cout << x << " * " << y << " = " << x * y << '\n';
    }
}