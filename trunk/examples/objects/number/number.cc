#include <iostream>
#include <errno.h>
#include <fstream>
#include <sys/stat.h>
#include "RpNumber.h"

size_t indent = 0;
size_t tabstop = 4;

int
printNumber(Rappture::Number *n)
{
    std::cout << "name: " << n->name() << std::endl;
    std::cout << "path: " << n->path() << std::endl;
    std::cout << "label: " << n->label() << std::endl;
    std::cout << "desc: " << n->desc() << std::endl;
    std::cout << "default: " << n->def() << std::endl;
    std::cout << "current: " << n->cur() << std::endl;
    std::cout << "min: " << n->min() << std::endl;
    std::cout << "max: " << n->max() << std::endl;
    std::cout << "units: " << n->units() << std::endl;
    std::cout << "xml:\n" << n->xml(indent,tabstop) << std::endl;
    return 0;
}

int testStringVal(
    const char *testname,
    const char *desc,
    const char *expected,
    const char *received)
{
    if ((!expected && received) ||
        (expected && !received) ||
        (expected && received && strcmp(expected,received) != 0)) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }
    return 0;
}

int testDoubleVal(
    const char *testname,
    const char *desc,
    double expected,
    double received)
{
    if (expected != received) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%g\"\n",expected);
        printf("\treceived \"%g\"\n",received);
        return 1;
    }
    return 0;
}

size_t
readFile (
    const char *filePath,
    const char **buf)
{
    if (buf == NULL) {
        fprintf(stderr,"buf is NULL while opening file \"%s\"", filePath);
        return 0;
    }

    FILE *f;
    f = fopen(filePath, "rb");
    if (f == NULL) {
        fprintf(stderr,"can't open \"%s\": %s", filePath, strerror(errno));
        return 0;
    }
    struct stat stat;
    if (fstat(fileno(f), &stat) < 0) {
        fprintf(stderr,"can't stat \"%s\": %s", filePath, strerror(errno));
        return 0;
    }
    off_t size;
    size = stat.st_size;
    char* memblock;
    memblock = new char [size+1];
    if (memblock == NULL) {
        fprintf(stderr,"can't allocate %zu bytes for file \"%s\": %s",
            size, filePath, strerror(errno));
        fclose(f);
        return 0;
    }

    size_t nRead;
    nRead = fread(memblock, sizeof(char), size, f);
    fclose(f);

    if (nRead != (size_t)size) {
        fprintf(stderr,"can't read %zu bytes from \"%s\": %s", size, filePath,
            strerror(errno));
        return 0;
    }

    memblock[size] = '\0';
    *buf = memblock;
    return nRead;
}
int number_0_0 ()
{
    const char *testdesc = "test number full constructor";
    const char *testname = "number_0_0";
    int retVal = 0;

    const char *name = "temp";
    const char *units = "K";
    double def = 300;
    double min = 0;
    double max = 1000;
    const char *label = "mylabel";
    const char *desc = "mydesc";

    Rappture::Number *n = NULL;

    n = new Rappture::Number(name,units,def,min,max,label,desc);

    if (n == NULL) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", testdesc);
        printf("\terror creating number object\n");
        retVal = 1;
    }

    retVal |= testStringVal(testname,testdesc,name,n->name());
    retVal |= testStringVal(testname,testdesc,units,n->units());
    retVal |= testDoubleVal(testname,testdesc,def,n->def());
    retVal |= testDoubleVal(testname,testdesc,min,n->min());
    retVal |= testDoubleVal(testname,testdesc,max,n->max());
    retVal |= testStringVal(testname,testdesc,label,n->label());
    retVal |= testStringVal(testname,testdesc,desc,n->desc());

    if (n != NULL) {
        delete n;
    }

    return retVal;
}

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

int main()
{
    number_0_0();
    number_0_1();
    number_1_0();

    return 0;
}
