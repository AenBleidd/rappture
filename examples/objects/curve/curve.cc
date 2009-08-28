#include <iostream>
#include "RpCurve.h"

size_t indent = 0;
size_t tabstop = 4;

int test(
    const char *testname,
    const char *desc,
    const char *expected,
    const char *received)
{
    if (strcmp(expected,received) != 0) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }
    return 0;
}

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

    std::cout << "xml: " << n->xml(indent,tabstop) << std::endl;
    return 0;
}

int curve_0_0 ()
{
    const char *desc = "test basic constructor";
    const char *testname = "curve_0_0";

    const char *expected = "output.curve(myid)";
    const char *received = NULL;

    Rappture::Curve c(expected);
    received = c.path();

    return test(testname,desc,expected,received);
}

int main()
{
    Rappture::Curve *n = NULL;
    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double z[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,91,100};

    n = new Rappture::Curve("output.curve(temperature)","mylabel",
                            "mydesc","mygroup");

    n->axis("xaxis","xlabel","xdesc","xunits","xscale",x,10);
    n->axis("yaxis","ylabel","ydesc","yunits","yscale",y,10);
    n->axis("zaxis","zlabel","zdesc","zunits","zscale",z,10);
    printCurve(n);
    n->delAxis("zlabel");
    n->delAxis("xzylabel");
    printCurve(n);

    delete n;
    return 0;
}
