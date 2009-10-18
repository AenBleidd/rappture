#include <iostream>
#include "RpLibObj.h"
#include "RpInt.h"
#include "RpChain.h"
#include "RpTest.h"

int library_0_0 ()
{
    const char *testdesc = "test creating library object with loadFile()";
    const char *testname = "library_0_0";
    const char *filePath = "library_0_0_in.xml";
    int retVal = 0;

    Rappture::Library lib;

    lib.loadFile(filePath);

    const char *expected = NULL;

    readFile(filePath, &expected);
    const char *received = lib.xml();

    retVal |= testStringVal(testname,testdesc,expected,received);

    delete expected;

    return retVal;
}

int library_0_1 ()
{
    const char *testdesc = "test creating library object with loadXml()";
    const char *testname = "library_0_1";
    int retVal = 0;

    Rappture::Library lib;

    const char *buf =
"<?xml version=\"1.0\"?>\n\
<run>\n\
    <input>\n\
        <number id=\"Ef\">\n\
            <about>\n\
                <label>Fermi Level</label>\n\
                <description>Energy at center of distribution.</description>\n\
            </about>\n\
            <units>eV</units>\n\
            <min>-10eV</min>\n\
            <max>10eV</max>\n\
            <default>0eV</default>\n\
            <current>5eV</current>\n\
            <preset>\n\
                <label>300K (room temperature)</label>\n\
                <value>300K</value>\n\
            </preset>\n\
            <preset>\n\
                <label>77K (liquid nitrogen)</label>\n\
                <value>77K</value>\n\
            </preset>\n\
            <preset>\n\
                <label>4.2K (liquid helium)</label>\n\
                <value>4.2K</value>\n\
            </preset>\n\
        </number>\n\
    </input>\n\
</run>\n\
";

    lib.loadXml(buf);

    const char *expected = buf;
    const char *received = lib.xml();

    retVal |= testStringVal(testname,testdesc,expected,received);

    return retVal;
}

int library_1_0 ()
{
    const char *testdesc = "test contains() function on empty library";
    const char *testname = "library_1_0";
    int retVal = 0;

    Rappture::Library lib;
    const Rp_Chain *contents = NULL;
    contents = lib.contains();

    int expected = 0;
    int received = Rp_ChainGetLength(contents);

    retVal |= testDoubleVal(testname,testdesc,expected,received);

    return retVal;
}

int library_1_1 ()
{
    const char *testdesc = "test contains() function on populated library";
    const char *testname = "library_1_1";
    int retVal = 0;

    Rappture::Library lib;
    const Rp_Chain *contents = NULL;
    const char *buf =
"<?xml version=\"1.0\"?>\
<run>\
    <input>\
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
        </number>\
    </input>\
</run>";

    lib.loadXml(buf);
    contents = lib.contains();

    int expected = 1;
    int received = Rp_ChainGetLength(contents);

    retVal |= testDoubleVal(testname,testdesc,expected,received);

    return retVal;
}

int library_2_0 ()
{
    const char *testdesc = "test value(), 0 hints";
    const char *testname = "library_2_0";
    int retVal = 0;

    Rappture::Library lib;
    const char *buf =
"<?xml version=\"1.0\"?>\
<run>\
    <input>\
        <number id=\"temperature\">\
            <about>\
                <label>Ambient temperature</label>\
                <description>Temperature of the environment.</description>\
            </about>\
            <units>K</units>\
            <min>0K</min>\
            <max>500K</max>\
            <default>300K</default>\
        </number>\
        <number id=\"Ef\">\
            <about>\
                <label>Fermi Level</label>\
                <description>Energy at center of distribution.</description>\
            </about>\
            <units>eV</units>\
            <min>-10eV</min>\
            <max>10eV</max>\
            <default>4eV</default>\
        </number>\
    </input>\
</run>";

    lib.loadXml(buf);

    double expected = 4.0;
    double received = 0.0;
    lib.value("Ef",&received,0);

    retVal |= testDoubleVal(testname,testdesc,expected,received);

    return retVal;
}

int library_2_1 ()
{
    const char *testdesc = "test value(), 1 hint, units=eV";
    const char *testname = "library_2_1";
    int retVal = 0;

    Rappture::Library lib;
    const char *buf =
"<?xml version=\"1.0\"?>\
<run>\
    <input>\
        <number id=\"temperature\">\
            <about>\
                <label>Ambient temperature</label>\
                <description>Temperature of the environment.</description>\
            </about>\
            <units>K</units>\
            <min>0K</min>\
            <max>500K</max>\
            <default>300K</default>\
        </number>\
        <number id=\"Ef\">\
            <about>\
                <label>Fermi Level</label>\
                <description>Energy at center of distribution.</description>\
            </about>\
            <units>eV</units>\
            <min>-10eV</min>\
            <max>10eV</max>\
            <default>4eV</default>\
        </number>\
    </input>\
</run>";

    lib.loadXml(buf);

    double expected = 4.0;
    double received = 0.0;
    lib.value("Ef",&received,1,"units=eV");

    retVal |= testDoubleVal(testname,testdesc,expected,received);

    return retVal;
}

int library_2_2 ()
{
    const char *testdesc = "test value(), 1 hint, units=J";
    const char *testname = "library_2_2";
    int retVal = 0;

    Rappture::Library lib;
    const char *buf =
"<?xml version=\"1.0\"?>\
<run>\
    <input>\
        <number id=\"temperature\">\
            <about>\
                <label>Ambient temperature</label>\
                <description>Temperature of the environment.</description>\
            </about>\
            <units>K</units>\
            <min>0K</min>\
            <max>500K</max>\
            <default>300K</default>\
        </number>\
        <number id=\"Ef\">\
            <about>\
                <label>Fermi Level</label>\
                <description>Energy at center of distribution.</description>\
            </about>\
            <units>eV</units>\
            <min>-10eV</min>\
            <max>10eV</max>\
            <default>4eV</default>\
        </number>\
    </input>\
</run>";

    lib.loadXml(buf);

    // double expected = 6.40871e-19;
    double expected = 4*1.602177e-19;
    double received = 0.0;
    lib.value("Ef",&received,1,"units=J");

    retVal |= testDoubleVal(testname,testdesc,expected,received);

    return retVal;
}

int library_2_3 ()
{
    const char *testdesc = "test value(), object does not exist";
    const char *testname = "library_2_3";
    int retVal = 0;

    Rappture::Library lib;
    const char *buf =
"<?xml version=\"1.0\"?>\
<run>\
    <input>\
        <number id=\"temperature\">\
            <about>\
                <label>Ambient temperature</label>\
                <description>Temperature of the environment.</description>\
            </about>\
            <units>K</units>\
            <min>0K</min>\
            <max>500K</max>\
            <default>300K</default>\
        </number>\
        <number id=\"Ef\">\
            <about>\
                <label>Fermi Level</label>\
                <description>Energy at center of distribution.</description>\
            </about>\
            <units>eV</units>\
            <min>-10eV</min>\
            <max>10eV</max>\
            <default>4eV</default>\
        </number>\
    </input>\
</run>";

    lib.loadXml(buf);

    // library should not change the value of received
    // if it cannot find the requested object
    double expected = -101.01;
    double received = -101.01;
    lib.value("EF",&received,1,"units=J");

    retVal |= testDoubleVal(testname,testdesc,1,lib.error());
    retVal |= testDoubleVal(testname,testdesc,expected,received);

    return retVal;
}

int main()
{
    library_0_0();
    library_0_1();
    library_1_0();
    library_2_0();
    library_2_1();
    library_2_2();
    library_2_3();

    return 0;
}
