#include <iostream>
#include "RpPlot.h"

int main()
{
    // plot object
    Rappture::Plot *p1 = NULL;

    // data arrays
    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double z[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,81,100};

    // number of points in x, y, z arrays
    size_t nPts = 10;

    // line format: green line, dotted line style, circle marker
    const char *fmt = "g:o"

    p1 = new Rappture::Plot();

    // line id handle, not really sure what to do with these yet
    size_t cid1 = 0;
    size_t cid2 = 0;

    // add line to the plot, with optional format and line name
    // line name can be used to retrieve cid later?
    cid1 = p1->add(nPts,x,y,fmt,"lineName");
    cid2 = p1->add(nPts,x,z,"b-*");
    p1->add(nPts,x,x);

    // might want to associate info with a specific cid?
    // info method implementable with varargs
    // use 1 call to add multiple properties
    /*
    p1->info("xlabel","Voltage",
              "xdesc","Voltage along the Gate",
             "xunits","Volt",
             "xscale","linear");
    */

    // or use multiple calls to add one property at a time
    p1->property("ylabel","Current");
    p1->property("ydesc","Current along the Drain");
    p1->property("yunits","Amp");
    p1->property("yscale","log");

    // number of curves in this plot
    size_t curveCnt = p1->count();

    // const array of line id's
    // const size_t *lineIds = p1->lines();

    // retrieve cid from curve name
    // may want to add cid to another plot?
    cid = p1->line("lineName");

    delete p1;

    return 0;
}
