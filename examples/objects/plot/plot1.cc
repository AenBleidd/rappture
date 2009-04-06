// matlab, matplotlib like interface
// maybe implementable with varargs?
// donno how to implement doubles and triples of args:
//  x,y,x,z
//  x,y,fmt,x,z,fmt2

#include <iostream>
#include "RpPlot.h"

int main()
{
    Rappture::Plot *p1 = NULL;
    Rappture::Plot *p2 = NULL;
    Rappture::Plot *p3 = NULL;
    Rappture::Plot *p4 = NULL;

    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double z[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,81,100};

    size_t nPts = 10;

    // green line, dotted line style, circle marker
    const char *fmt = "g:o"

    p1 = new Rappture::Plot(nPts,x,y);
    p2 = new Rappture::Plot(nPts,x,y,fmt);
    p3 = new Rappture::Plot(nPts,x,z,x,y,"g:o");
    p4 = new Rappture::Plot(nPts,x,z,nPts,x,y,nPts,x,x);

    delete p1;
    delete p2;
    delete p3;
    delete p4;

    return 0;
}
