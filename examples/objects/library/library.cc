#include <iostream>
#include <errno.h>
#include "RpLibObj.h"
#include "RpInt.h"
#include "RpChain.h"

int library_0_0 ()
{
    const char *testdesc = "test creating library object";
    const char *testname = "library_0_0";
    const char *filePath = "library_0_0_in.xml";
    int retVal = 0;

    Rappture::Library lib;
    const Rp_Chain *contents = NULL;

    lib.loadFile(filePath);
    contents = lib.contains();

    std::printf("lib contains %d item(s)\n", Rp_ChainGetLength(contents));

    return retVal;
}

int library_1_0 ()
{
    const char *testdesc = "test creating library object";
    const char *testname = "library_1_0";
    const char *filePath = "library_0_0_in.xml";
    int retVal = 0;

    Rappture::Library lib;
    const Rp_Chain *contents = NULL;

    lib.loadFile(filePath);

    const char *xmltext = lib.xml();
    printf("xmltext = %s\n",xmltext);

    return retVal;
}

/*
int number_0_1 ()
{
    const char *testdesc = "test number constuctor";
    const char *testname = "number_0_0";
    int retVal = 0;

    const char *name = "eV";
    const char *units = "eV";
    double def = 1.4;

    Rappture::Number *n = NULL;

    n = new Rappture::Number(name,units,def);

    if (n == NULL) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", testdesc);
        printf("\terror creating number object\n");
        retVal = 1;
    }

    retVal |= testStringVal(testname,testdesc,name,n->name());
    retVal |= testStringVal(testname,testdesc,units,n->units());
    retVal |= testDoubleVal(testname,testdesc,def,n->def());

    if (n != NULL) {
        delete n;
    }

    return retVal;
}

int number_1_0 ()
{
    const char *testdesc = "test constructing number from xml";
    const char *testname = "number_1_0";
    const char *filePath = "number_1_0_in.xml";
    int retVal = 0;

    const char *buf = NULL;
    readFile(filePath,&buf);

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.xml(buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    retVal |= testDoubleVal(testname,testdesc,min,n.min());
    retVal |= testDoubleVal(testname,testdesc,max,n.max());
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}
*/

int main()
{
    library_0_0();
    // library_0_1();
    library_1_0();

    return 0;
}
