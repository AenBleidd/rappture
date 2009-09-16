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

int curve_0_0 ()
{
    const char *desc = "test basic constructor";
    const char *testname = "curve_0_0";

    const char *expected = "myid";
    const char *received = NULL;

    Rappture::Curve c(expected);
    received = c.name();

    return test(testname,desc,expected,received);
}

int curve_1_0 ()
{
    const char *desc = "test generating xml text for curve object";
    const char *testname = "curve_1_0";

    const char *expected = "<?xml version=\"1.0\"?>\n\
<curve id=\"myid\">\n\
    <about>\n\
        <group>mygroup</group>\n\
        <label>mylabel</label>\n\
        <description>mydesc</description>\n\
        <type>(null)</type>\n\
    </about>\n\
    <xaxis>\n\
        <label>xlabel</label>\n\
        <description>xdesc</description>\n\
        <units>xunits</units>\n\
        <scale>xscale</scale>\n\
    </xaxis>\n\
    <yaxis>\n\
        <label>ylabel</label>\n\
        <description>ydesc</description>\n\
        <units>yunits</units>\n\
        <scale>yscale</scale>\n\
    </yaxis>\n\
    <component>\n\
        <xy>         1         1\n\
         2         4\n\
         3         9\n\
         4        16\n\
         5        25\n\
         6        36\n\
         7        49\n\
         8        64\n\
         9        81\n\
        10       100\n\
</xy>\n\
    </component>\n\
</curve>\n\
";
    const char *received = NULL;

    Rappture::Curve *c = NULL;
    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,81,100};

    c = new Rappture::Curve("myid","mylabel","mydesc","mygroup");

    c->axis("xaxis","xlabel","xdesc","xunits","xscale",x,10);
    c->axis("yaxis","ylabel","ydesc","yunits","yscale",y,10);

    Rappture::ClientDataXml xmldata;
    xmldata.indent = indent;
    xmldata.tabstop = tabstop;
    xmldata.retStr = NULL;
    c->dump(Rappture::RPCONFIG_XML,&xmldata);
    received = xmldata.retStr;

    return test(testname,desc,expected,received);
}

int main()
{
    curve_0_0 ();
    curve_1_0 ();
    return 0;
}
