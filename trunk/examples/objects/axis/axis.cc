#include <iostream>
#include "RpArray1D.h"

int
printAxis(Rappture::Array1D *n)
{
    std::cout << "path: " << n->path() << std::endl;
    std::cout << "label: " << n->label() << std::endl;
    std::cout << "desc: " << n->desc() << std::endl;
    std::cout << "nmemb: " << n->nmemb() << std::endl;
    std::cout << "min: " << n->min() << std::endl;
    std::cout << "max: " << n->max() << std::endl;
    std::cout << "units: " << n->units() << std::endl;

    const double *d = n->data();

    for (size_t i = 0; i < n->nmemb(); i++) {
        std::cout << "d[" << i << "] = " << d[i] << std::endl;
    }

    return 0;
}

int main()
{
    Rappture::Array1D *n = NULL;
    double d[] = {70,400,199,502,832924};

    n = new Rappture::Array1D("output.axis(temperature)",d,5,"mylabel",
                            "mydesc","K","linear");

    printAxis(n);

    Rappture::Array1D n1("output.axis(t)",d,5);
    printAxis(&n1);

    delete n;

    return 0;
}
