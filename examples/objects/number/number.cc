#include <iostream>
#include "RpNumber.h"
#include "RpTest.h"

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
    // std::cout << "xml:\n" << n->xml(indent,tabstop) << std::endl;
    return 0;
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
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <default>0eV</default>\
    <current>5eV</current>\
    <preset>\
        <value>300K</value>\
        <label>300K (room temperature)</label>\
    </preset>\
    <preset>\
        <value>77K</value>\
        <label>77K (liquid nitrogen)</label>\
    </preset>\
    <preset>\
        <value>4.2K</value>\
        <label>4.2K (liquid helium)</label>\
    </preset>\
</number>";

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_1 ()
{
    const char *testdesc = "test constructing number from xml, no min";
    const char *testname = "number_1_1";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <max>10eV</max>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_2 ()
{
    const char *testdesc = "test constructing number from xml, no max";
    const char *testname = "number_1_2";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";


    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_3 ()
{
    const char *testdesc = "test constructing number from xml, no min, no max";
    const char *testname = "number_1_3";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";


    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_4 ()
{
    const char *testdesc = "test constructing number from xml, no cur";
    const char *testname = "number_1_4";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <default>0eV</default>\
</number>";


    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 0;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    // if no current value is provided it defaults to 0.0.
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_5 ()
{
    const char *testdesc = "test constructing number from xml, no def";
    const char *testname = "number_1_5";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <current>5eV</current>\
</number>";


    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    // if no default value is provided it defaults to 0.0.
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_6 ()
{
    const char *testdesc = "test constructing number from xml, no def, no cur";
    const char *testname = "number_1_6";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
</number>";


    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 0;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_7 ()
{
    const char *testdesc = "test constructing number from xml, no def cur min max";
    const char *testname = "number_1_7";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
</number>";


    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 0;
    double min = 0;
    double max = 0;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

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

int number_1_8 ()
{
    const char *testdesc = "test constructing number from xml, no explicit units";
    const char *testname = "number_1_8";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    // units are implied from min, max, def, cur values
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_9 ()
{
    const char *testdesc = "test constructing number from xml, no description";
    const char *testname = "number_1_9";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_10 ()
{
    const char *testdesc = "test constructing number from xml, no label";
    const char *testname = "number_1_10";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_11 ()
{
    const char *testdesc = "test constructing number from xml, no label description";
    const char *testname = "number_1_11";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
    </about>\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "";
    const char *desc = "";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    // if no default value and no current value is provided
    // both default to 0.0.
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    retVal |= testDoubleVal(testname,testdesc,min,n.min());
    retVal |= testDoubleVal(testname,testdesc,max,n.max());
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_12 ()
{
    const char *testdesc = "test constructing number from xml, no about label description";
    const char *testname = "number_1_12";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <units>eV</units>\
    <min>-10eV</min>\
    <max>10eV</max>\
    <default>0eV</default>\
    <current>5eV</current>\
</number>";

    const char *name = "Ef";
    const char *units = "eV";
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "";
    const char *desc = "";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    // if no default value and no current value is provided
    // both default to 0.0.
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    retVal |= testDoubleVal(testname,testdesc,min,n.min());
    retVal |= testDoubleVal(testname,testdesc,max,n.max());
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_13 ()
{
    const char *testdesc = "test constructing number from xml, no def cur min max units";
    const char *testname = "number_1_13";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
</number>";


    const char *name = "Ef";
    const char *units = NULL;
    double def = 0;
    double cur = 0;
    double min = 0;
    double max = 0;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    // if no default value and no current value is provided
    // both default to 0.0.
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    retVal |= testDoubleVal(testname,testdesc,min,n.min());
    retVal |= testDoubleVal(testname,testdesc,max,n.max());
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

int number_1_14 ()
{
    const char *testdesc = "test constructing number from xml, only name";
    const char *testname = "number_1_14";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
</number>";


    const char *name = "Ef";
    const char *units = NULL;
    double def = 0;
    double cur = 0;
    double min = 0;
    double max = 0;
    const char *label = "";
    const char *desc = "";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

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

int number_1_15 ()
{
    const char *testdesc = "test constructing number from xml, no info";
    const char *testname = "number_1_15";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number>\
</number>";


    const char *name = "";
    const char *units = NULL;
    double def = 0;
    double cur = 0;
    double min = 0;
    double max = 0;
    const char *label = "";
    const char *desc = "";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

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

int number_1_16 ()
{
    const char *testdesc = "test constructing number from xml, no explicit or implied units";
    const char *testname = "number_1_16";
    int retVal = 0;

    const char *buf =
"<?xml version=\"1.0\"?>\
<number id=\"Ef\">\
    <about>\
        <label>Fermi Level</label>\
        <description>Energy at center of distribution.</description>\
    </about>\
    <min>-10</min>\
    <max>10</max>\
    <default>0</default>\
    <current>5</current>\
</number>";

    const char *name = "Ef";
    const char *units = NULL;
    double def = 0;
    double cur = 5;
    double min = -10;
    double max = 10;
    const char *label = "Fermi Level";
    const char *desc = "Energy at center of distribution.";

    Rappture::Number n;

    n.configure(Rappture::RPCONFIG_XML,(void*)buf);

    retVal |= testStringVal(testname,testdesc,name,n.name());
    retVal |= testStringVal(testname,testdesc,units,n.units());
    retVal |= testDoubleVal(testname,testdesc,def,n.def());
    retVal |= testDoubleVal(testname,testdesc,cur,n.cur());
    if (n.minset()) {
        retVal |= testDoubleVal(testname,testdesc,min,n.min());
    }
    if (n.maxset()) {
        retVal |= testDoubleVal(testname,testdesc,max,n.max());
    }
    retVal |= testStringVal(testname,testdesc,label,n.label());
    retVal |= testStringVal(testname,testdesc,desc,n.desc());

    return retVal;
}

// FIXME: more tests:
// what if units in min, max, def, cur differ from units()
// what if you can't convert from units in min, max, def, cur to units()
// what if units in presets differ from units()
// test case for value() function when only cur was configured
// test case for value() function when only def was configured
// test case for vvalue() function with no hints
// test case for vvalue() function with "units=" hint
// test case for setting up object, what is cur when only def is provided
// test case for setting up object, what is def when only cur is provided

int main()
{
    number_0_0();
    number_0_1();
    number_1_0();
    number_1_1();
    number_1_2();
    number_1_3();
    number_1_4();
    number_1_5();
    number_1_6();
    number_1_7();
    number_1_8();
    number_1_9();
    number_1_10();
    number_1_11();
    number_1_12();
    number_1_13();
    number_1_14();
    number_1_15();
    number_1_16();

    return 0;
}
