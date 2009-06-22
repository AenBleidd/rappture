#include <iostream>
#include "RpArray1D.h"


int test(
    const char *testname,
    const char *desc,
    const char *expected,
    const char *received)
{
/*
    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }
*/
    return 0;
}

int
printAxis(Rappture::Array1D *n)
{
    std::cout << "name: " << n->name() << std::endl;
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

int axis_0_0 ()
{
    const char *desc = "";
    const char *testname = "axis_0_0";


    const char *expected = NULL;
    const char *received = NULL;

    return test(testname,desc,expected,received);
}

int main()
{
    Rappture::Array1D *n = NULL;
    double d[] = {70,400,199,502,832924};

    n = new Rappture::Array1D(d,5);
    n->name("output.axis(temperature)");
    n->label("mylabel");
    n->desc("mydesc");
    n->units("K");
    n->scale("linear");

    printAxis(n);

    Rappture::Array1D n1(d,5);
    n1.name("output.axis(t)");
    printAxis(&n1);

    delete n;

    axis_0_0();

    return 0;
}
