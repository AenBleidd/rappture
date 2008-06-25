#include <iostream>
#include <cstdio>
#include "RpDXWriter.h"

int main()
{
    Rappture::DXWriter d;
    float i = 0.0;
//    int rank = 3;
    size_t p[] = {3,5,5};
//    float origin[] = {0.0,0.0,0.0};
//    float delta[] = {1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0};

//    d.rank(rank);
//    d.origin(origin);
//    d.delta(delta);
    d.counts(p);

    for(size_t z = 0; z < p[2]; z++) {
        for(size_t y = 0; y < p[1]; y++) {
            for(size_t x = 0; x < p[0]; x++) {
                i = x*y*z;
                d.data(&i);
            }
        }
    }

    FILE *fp = NULL;
    fp = fopen("myDXFile.dx","w");
    d.write(fp);
    fclose(fp);

    return 0;
}
