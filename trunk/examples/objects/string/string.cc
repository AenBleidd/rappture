#include <iostream>
#include "RpString.h"

int
printString(Rappture::String *n)
{
    std::cout << "path: " << n->path() << std::endl;
    std::cout << "label: " << n->label() << std::endl;
    std::cout << "desc: " << n->desc() << std::endl;
    std::cout << "hints: " << n->hints() << std::endl;
    std::cout << "default: " << n->def() << std::endl;
    std::cout << "current: " << n->cur() << std::endl;
    std::cout << "width: " << n->width() << std::endl;
    std::cout << "height: " << n->height() << std::endl;
    return 0;
}

int main()
{
    Rappture::String *n = NULL;

    n = new Rappture::String("input.string(temperature)",
                             "myString Value","mylabel",
                             "mydesc","myhint",20,30);

    printString(n);

    Rappture::String n1("input.string(eV)","eV");
    printString(&n1);

    delete n;

    return 0;
}
