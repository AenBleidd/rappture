#include <iostream>
#include "RpHistogram.h"

int main()
{
    // histogram object
    Rappture::Histogram *h1 = NULL;
    Rappture::Histogram *h2 = NULL;
    Rappture::Histogram *h3 = NULL;

    // data arrays
    double bins[] = {1,2,3,4,5,6,7,8,9,10};
    double y[] = {1,4,9,16,25,36,49,64,81,100};
    // double z[] = {1,8,27,64,125,216,343,512,729,1000};
    size_t binWidths[] = {1,1,1,2,1,1,1,1,1,1};

    // number of points in x, y, z arrays
    size_t nPts = 10;
    size_t nBins = 10;

    h1 = new Rappture::Histogram();
    h1->xaxis("xlabel1","xdesc1","xunits1",bins,nBins);
    h1->yaxis("ylabel1","ydesc1","yunits1",y,nPts);
    h1->binWidths(binWidths,nBins);
    h1->marker(Rappture::Histogram::x,5,"This is five","-foreground red -linewidth 2");

    h2 = new Rappture::Histogram(y,nPts,bins,nBins);
    h2->xaxis("xlabel2","xdesc2","xunits2");
    h2->yaxis("ylabel2","ydesc2","yunits2");
    h1->marker(Rappture::Histogram::y,2,"This is two","-foreground blue -linewidth 1");

    double rangeMin = 1.0;
    double rangeMax = 10.0;
    h3 = new Rappture::Histogram(y,nPts,rangeMin,rangeMax,nBins);
    h3->xaxis("xlabel3","xdesc3","xunits3");
    h3->yaxis("ylabel3","ydesc3","yunits3");


    std::printf("xml: %s\n",h1->xml());
    std::printf("xml: %s\n",h2->xml());
    std::printf("xml: %s\n",h3->xml());

    return 0;
}
