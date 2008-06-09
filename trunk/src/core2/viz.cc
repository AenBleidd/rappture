#include <stdio.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <string>
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"

/* Load a 3D volume from a dx-format file
 */
void
load_volume_file(int index, char *fname) {
    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;
    std::ifstream fin(fname);

    int isrect = 1;

    do {
        fin.getline(line,sizeof(line)-1);
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
                isrect = 1;
            }
            else if (sscanf(start, "object %d class array type float rank 1 shape 3 items %d data follows", &dummy, &nxy) == 2) {
                std::ofstream ftmp("tmppts");
                double xx, yy, zz;
                isrect = 0;
                for (int i=0; i < nxy; i++) {
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &xx, &yy, &zz) == 3) {
                        xymesh.addNode( Rappture::Node2D(xx,yy) );
                        ftmp << xx << " " << yy << std::endl;
                    }
                }
                ftmp.close();

                if (system("voronoi -t < tmppts > tmpcells") == 0) {
                    int cx, cy, cz;
                    std::ifstream ftri("tmpcells");
                    while (!ftri.eof()) {
                        ftri.getline(line,sizeof(line)-1);
                        if (sscanf(line, "%d %d %d", &cx, &cy, &cz) == 3) {
                            xymesh.addCell(cx, cy, cz);
                        }
                    }
                    ftri.close();
                } else {
                    std::cerr << "WARNING: triangularization failed" << std::endl;
                }
            }
            else if (sscanf(start, "object %d class regulararray count %d", &dummy, &nz) == 2) {
                // found z-grid
            }
            else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            }
            else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                if (ddx != 0.0) { dx = ddx; }
                else if (ddy != 0.0) { dy = ddy; }
                else if (ddz != 0.0) { dz = ddz; }
            }
            else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows", &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                else if (!isrect && (npts != nxy*nz)) {
                    std::cerr << "inconsistent data: expected " << nxy*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                break;
            }
        }
    } while (!fin.eof());

    // read data points
    if (!fin.eof()) {
        if (isrect) {
            Rappture::Mesh1D xgrid(x0, x0+nx*dx, nx);
            Rappture::Mesh1D ygrid(y0, y0+ny*dy, ny);
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldRect3D field(xgrid, ygrid, zgrid);

            double dval;
            int nread = 0;
            while (!fin.eof() && nread < npts) {
                if (!(fin >> dval).fail()) {
                    field.define(nread++, dval);
                }
            }

            // make sure that we read all of the expected points
            if (nread != nx*ny*nz) {
                std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << nread << " points" << std::endl;
                return;
            }

            // figure out a good mesh spacing
            int nsample = 100;
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);
            float *data = new float[nx*ny*nz];
            std::cout << "generating " << nx << "x" << ny << "x" << nz << " = " << nx*ny*nz << " points" << std::endl;

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        data[ngen++] = field.value(xval,yval,zval);
                    }
                }
            }
        } else {
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldPrism3D field(xymesh, zgrid);

            double dval;
            int nread = 0;
            while (!fin.eof() && nread < npts) {
                if (!(fin >> dval).fail()) {
                    field.define(nread++, dval);
                }
            }

            // make sure that we read all of the expected points
            if (nread != nxy*nz) {
                std::cerr << "inconsistent data: expected " << nxy*nz << " points but found " << nread << " points" << std::endl;
                return;
            }

            // figure out a good mesh spacing
            int nsample = 100;
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);
            float *data = new float[nx*ny*nz];
            std::cout << "generating " << nx << "x" << ny << "x" << nz << " = " << nx*ny*nz << " points" << std::endl;

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        data[ngen++] = field.value(xval,yval,zval);
                    }
                }
            }
        }
    } else {
        std::cerr << "WARNING: data not found in file " << fname << std::endl;
    }

    //load_volume(index, nx, ny, nz, 1, data);
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: viz file" << std::endl;
        exit(1);
    }
    load_volume_file(0, argv[1]);
    exit(0);
}
