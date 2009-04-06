#include <iostream>
#include "RpPlot.h"

int main()
{
    Rappture::Plot *p0 = NULL;
    Rappture::Plot *p1 = NULL;
    Rappture::Plot *p2 = NULL;
    Rappture::Plot *p3 = NULL;
    Rappture::Plot *p4 = NULL;

    Rappture::Plot::lineInfo *li1;
    Rappture::Plot::lineInfo *li2;

    double x[] = {1,2,3,4,5,6,7,8,9,10};
    double z[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,81,100};

    size_t nPts = 10;

    li1 = new Rappture::Plot::lineInfo(
    li.xlabel("xlabel");
    li.xdesc("xdesc");
    li.ylabel("ylabel");

    p0 = new Rappture::Plot("blahPlot");
    p0->addAxis("xlabel","xdesc","xunits","xscale",x,nPts);
    p0->addAxis("ylabel","ydesc","yunits","yscale",y,nPts);

    p1 = new Rappture::Plot(nPts,x,y);
    p2 = new Rappture::Plot(nPts,x,y,fmt);
    p3 = new Rappture::Plot(nPts,x,z,x,y,fmt);
    p4 = new Rappture::Plot(nPts,x,z,nPts,x,y,nPts,x,x);

    p0->addAxis("zlabel","zdesc","zunits","zscale",z,10);

    printCurve(n);
    n->delAxis("zlabel");
    n->delAxis("xzylabel");
    printCurve(n);

    delete n;
    return 0;
}
