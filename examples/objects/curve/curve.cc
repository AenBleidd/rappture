#include <iostream>
#include "RpCurve.h"

int
printCurve(Rappture::Curve *n)
{
    std::cout << "path: " << n->path() << std::endl;
    std::cout << "label: " << n->label() << std::endl;
    std::cout << "desc: " << n->desc() << std::endl;
    std::cout << "group: " << n->group() << std::endl;
    std::cout << "dims: " << n->dims() << std::endl;

    const Rappture::Array1D *a = NULL;

    for (size_t i = 0; i < n->dims(); i++) {
        a = n->getNthAxis(i);
        std::cout << "a[" << i << "] = " << a->label() << std::endl;
    }

    return 0;
}

int main()
{
    Rappture::Curve *n = NULL;
    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double z[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,91,100};

    n = new Rappture::Curve("output.curve(temperature)","mylabel",
                            "mydesc","mygroup");

    n->axis("xlabel","xdesc","xunits","xscale",x,10);
    n->axis("ylabel","ydesc","yunits","yscale",y,10);
    n->axis("zlabel","zdesc","zunits","zscale",z,10);
    printCurve(n);
    n->delAxis("zlabel");
    n->delAxis("xzylabel");
    printCurve(n);

    delete n;
    return 0;
}
