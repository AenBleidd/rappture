#include <iostream>
#include "RpNumber.h"

int
printNumber(Rappture::Number *n)
{
    std::cout << "path: " << n->path() << std::endl;
    std::cout << "label: " << n->label() << std::endl;
    std::cout << "desc: " << n->desc() << std::endl;
    std::cout << "default: " << n->def() << std::endl;
    std::cout << "current: " << n->cur() << std::endl;
    std::cout << "min: " << n->min() << std::endl;
    std::cout << "max: " << n->max() << std::endl;
    std::cout << "units: " << n->units() << std::endl;
    std::cout << "xml: " << n->xml() << std::endl;
    return 0;
}

int main()
{
    Rappture::Number *n = NULL;

    n = new Rappture::Number("input.number(temperature)","K",300,
                             0,1000,"mylabel","mydesc");

    printNumber(n);

    Rappture::Number n1("input.number(eV)","eV",1.4);
    printNumber(&n1);

    delete n;

    return 0;
}
