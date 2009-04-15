#include <iostream>
#include "RpPlot.h"

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

    // retrieve curve from curve name
    // may want to add curve to another plot
    // or just read through the values
    Rappture::Curve *c = p1->curve("curve1");

    const double *ax = NULL;
    const double *ay = NULL;
    size_t xlen = 0;
    size_t ylen = 0;

    xlen = c->data(Rappture::Plot::xaxis,&ax);
    ylen = c->data(Rappture::Plot::yaxis,&ay);

    std::printf("xlen = %zu\nylen = %zu\n",xlen,ylen);
    std::printf("fmt = %s\n",c->propstr(Rappture::Plot::format,NULL));

    if (   (ax != NULL)
        && (ay != NULL)) {
        for (size_t i = 0; (i < xlen) && (i < ylen); i++) {
            std::printf("x = %g  y = %g\n",ax[i],ay[i]);
        }
    }

    delete p1;

    return 0;
}
