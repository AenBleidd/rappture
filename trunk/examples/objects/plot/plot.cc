#include <iostream>
#include "RpPlot.h"

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

int plot_0_0 ()
{
    const char *desc = "test basic plot object constructor";
    const char *testname = "plot_0_0";

    const char *expected = "run";
    const char *received = NULL;

    Rappture::Plot p;
    received = p.path();

    return test(testname,desc,expected,received);
}

int plot_1_0 ()
{
    const char *desc = "test generating xml text for single curve";
    const char *testname = "plot_1_0";

    const char *expected = "<?xml version=\"1.0\"?>\n\
<run>\n\
    <curve id=\"myid\">\n\
        <about>\n\
            <group>plotlabel</group>\n\
            <label>plotlabel</label>\n\
            <description>describe plot here</description>\n\
            <type>(null)</type>\n\
        </about>\n\
        <xaxis>\n\
            <label>Voltage</label>\n\
            <description>Voltage along the Gate</description>\n\
            <units>Volt</units>\n\
            <scale>linear</scale>\n\
        </xaxis>\n\
        <yaxis>\n\
            <label>Current</label>\n\
            <description>Current along the Drain</description>\n\
            <units>Amp</units>\n\
            <scale>log</scale>\n\
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
</run>\n\
";
    const char *received = NULL;

    Rappture::Plot p;
    size_t npts = 10;
    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,81,100};

    p.add(npts, x, y, NULL, "myid");

    p.propstr("label","plotlabel");
    p.propstr("desc","describe plot here");
    p.propstr("xlabel","Voltage");
    p.propstr("xdesc","Voltage along the Gate");
    p.propstr("xunits","Volt");
    p.propstr("xscale","linear");
    p.propstr("ylabel","Current");
    p.propstr("ydesc","Current along the Drain");
    p.propstr("yunits","Amp");
    p.propstr("yscale","log");

    Rappture::ClientDataXml xmldata;
    xmldata.indent = indent;
    xmldata.tabstop = tabstop;
    xmldata.retStr = NULL;
    p.dump(Rappture::RPCONFIG_XML,&xmldata);
    received = xmldata.retStr;

    return test(testname,desc,expected,received);
}

int main()
{
    plot_0_0 ();
    plot_1_0 ();
    return 0;
}

/*
int main()
{
    // plot object
    Rappture::Plot *p1 = NULL;

    // data arrays
    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,81,100};
    double z[] = {1,8,27,64,125,216,343,512,729,1000};

    // number of points in x, y, z arrays
    size_t nPts = 10;

    // line format: green line, dotted line style, circle marker
    const char *fmt = "g:o";

    p1 = new Rappture::Plot();

    // add 3 curves to the plot, with format and curve name
    // x vs y, x vs z, x vs x
    // curve name can be used to retrieve curve pointer later
    // blank format means autogenerate the format
    p1->add(nPts,x,y,fmt,"curve1");
    p1->add(nPts,x,z,"b-*","curve2");
    p1->add(nPts,x,x,"","curve3");

    p1->propstr("xlabel","Voltage");
    p1->propstr("xdesc","Voltage along the Gate");
    p1->propstr("xunits","Volt");
    p1->propstr("xscale","linear");
    p1->propstr("ylabel","Current");
    p1->propstr("ydesc","Current along the Drain");
    p1->propstr("yunits","Amp");
    p1->propstr("yscale","log");

    // number of curves in this plot
    size_t curveCnt = p1->count();
    std::printf("curveCnt = %zu\n",curveCnt);

    size_t indent = 0;
    size_t tabstop = 4;

    std::printf("xml: %s\n",p1->xml(indent,tabstop));

    delete p1;

    return 0;
}
*/
