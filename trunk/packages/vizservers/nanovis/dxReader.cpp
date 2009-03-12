 
/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
 *
 *  dxReader.cpp 
 *      This module contains openDX readers for 2D and 3D volumes.
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Michael McLennan <mmclennan@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

// common dx functions
#include "dxReaderCommon.h"

#include <stdio.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "Nv.h"

#include "nanovis.h"
#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"

//transfer function headers
#include "ZincBlendeVolume.h"
#include "NvZincBlendeReconstructor.h"

#define  _LOCAL_ZINC_TEST_ 0

/*
 * Load a 3D vector field from a dx-format file
 */
void
load_vector_stream2(int volindex, std::istream& fin)
{
    int dummy, nx, ny, nz, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    dx = dy = dz = 0.0;         // Suppress compiler warning.
    x0 = y0 = z0 = 0.0;		// May not have an origin line.
    while (!fin.eof()) {
        fin.getline(line, sizeof(line) - 1);
        if (fin.fail()) {
            //return result.error("error in data stream");
            return;
        }
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                printf("w:%d h:%d d:%d\n", nx, ny, nz);
                // found grid size
            } else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            } else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                if (ddx != 0.0) {
                    dx = ddx;
                } else if (ddy != 0.0) {
                    dy = ddy;
                } else if (ddz != 0.0) {
                    dz = ddz;
                }
            } else if (sscanf(start, "object %d class array type %s shape 3 rank 1 items %d data follows", &dummy, type, &npts) == 3) {
                printf("point %d\n", npts);
                if (npts != nx*ny*nz) {
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                break;
            }
        }
    }

    // read data points
    float* srcdata = new float[nx * ny * nz * 3];
    if (!fin.eof()) {
        double vx, vy, vz, vm;
        double max_x = -1e21, min_x = 1e21;
        double max_y = -1e21, min_y = 1e21;
        double max_z = -1e21, min_z = 1e21;
        double max_mag = -1e21, min_mag = 1e21;
        int nread = 0;
        for (int ix=0; ix < nx; ix++) {
            for (int iy=0; iy < ny; iy++) {
                for (int iz=0; iz < nz; iz++) {
                    if (fin.eof() || nread > npts) {
                        break;
                    }
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &vx, &vy, &vz) == 3) {
                        int nindex = (iz*nx*ny + iy*nx + ix) * 3;
                        srcdata[nindex] = vx;
                        //if (srcdata[nindex] > max_x) max_x = srcdata[nindex];
                        //if (srcdata[nindex] < min_x) min_x = srcdata[nindex];
                        ++nindex;

                        srcdata[nindex] = vy;
                        //if (srcdata[nindex] > max_y) max_y = srcdata[nindex];
                        //if (srcdata[nindex] < min_y) min_y = srcdata[nindex];
                        ++nindex;

                        srcdata[nindex] = vz;
                        //if (srcdata[nindex] > max_z) max_z = srcdata[nindex];
                        //if (srcdata[nindex] < min_z) min_z = srcdata[nindex];

                        vm = sqrt(vx*vx + vy*vy + vz*vz);
                        if (vm > max_mag) {
                            max_mag = vm;
                        }
                        if (vm < min_mag) {
                            min_mag = vm;
                        }

                        ++nread;
                    }
                }
            }
        }

        // make sure that we read all of the expected points
        if (nread != nx*ny*nz) {
            std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << nread << " points" << std::endl;
            return;
        }

        float *data = new float[4*nx*ny*nz];
        memset(data, 0, sizeof(float) * 4 * nx * ny * nz);

        std::cout << "generating " << nx << "x" << ny << "x" << nz << " = " << nx*ny*nz << " points" << std::endl;

        // generate the uniformly sampled data that we need for a volume
        double nzero_min = 0.0;
        int ngen = 0;
        int nindex = 0;
        for (int iz=0; iz < nz; iz++) {
            for (int iy=0; iy < ny; iy++) {
                for (int ix=0; ix < nx; ix++) {

                    vx = srcdata[nindex++];
                    vy = srcdata[nindex++];
                    vz = srcdata[nindex++];

                    double vm;
                    vm = sqrt(vx*vx + vy*vy + vz*vz);

                    data[ngen] = vm / max_mag; ++ngen;
                    data[ngen] = vx /(2.0*max_mag) + 0.5; ++ngen;
                    data[ngen] = vy /(2.0*max_mag) + 0.5; ++ngen;
                    data[ngen] = vz /(2.0*max_mag) + 0.5; ++ngen;
                }
            }
        }

        Volume *volPtr;
        volPtr = NanoVis::load_volume(volindex, nx, ny, nz, 4, data, min_mag, max_mag,
                    0);

        volPtr->xAxis.SetRange(x0, x0 + nx);
        volPtr->yAxis.SetRange(y0, y0 + ny);
        volPtr->zAxis.SetRange(z0, z0 + nz);
        volPtr->wAxis.SetRange(min_mag, max_mag);
        volPtr->update_pending = true;
        delete [] data;
    } else {
        std::cerr << "WARNING: data not found in stream" << std::endl;
    }
}
void
load_vector_stream(int index, std::istream& fin)
{
    int dummy, nx, ny, nz, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    dx = dy = dz = 0.0;         // Suppress compiler warning.
    x0 = y0 = z0 = 0.0;		// May not have an origin line.
    while (!fin.eof()) {
        fin.getline(line, sizeof(line) - 1);
        if (fin.fail()) {
            //return result.error("error in data stream");
            return;
        }
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
		printf("w:%d h:%d d:%d\n", nx, ny, nz);
                // found grid size
            } else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            } else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                if (ddx != 0.0) {
                    dx = ddx;
                } else if (ddy != 0.0) {
                    dy = ddy;
                } else if (ddz != 0.0) {
                    dz = ddz;
                }
            } else if (sscanf(start, "object %d class array type %s shape 3 rank 1 items %d data follows", &dummy, type, &npts) == 3) {
		printf("point %d\n", npts);
                if (npts != nx*ny*nz) {
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                break;
            }
        }
    }

    // read data points
    if (!fin.eof()) {
        Rappture::Mesh1D xgrid(x0, x0+nx*dx, nx);
        Rappture::Mesh1D ygrid(y0, y0+ny*dy, ny);
        Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
        Rappture::FieldRect3D xfield(xgrid, ygrid, zgrid);
        Rappture::FieldRect3D yfield(xgrid, ygrid, zgrid);
        Rappture::FieldRect3D zfield(xgrid, ygrid, zgrid);

        double vx, vy, vz;
        int nread = 0;
        for (int ix=0; ix < nx; ix++) {
            for (int iy=0; iy < ny; iy++) {
                for (int iz=0; iz < nz; iz++) {
                    if (fin.eof() || nread > npts) {
                        break;
                    }
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &vx, &vy, &vz) == 3) {
                        int nindex = iz*nx*ny + iy*nx + ix;
                        xfield.define(nindex, vx);
                        yfield.define(nindex, vy);
                        zfield.define(nindex, vz);
                        nread++;
                    }
                }
            }
        }

        // make sure that we read all of the expected points
        if (nread != nx*ny*nz) {
            std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << nread << " points" << std::endl;
            return;
        }

        // figure out a good mesh spacing
        int nsample = 30;
        dx = xfield.rangeMax(Rappture::xaxis) - xfield.rangeMin(Rappture::xaxis);
        dy = xfield.rangeMax(Rappture::yaxis) - xfield.rangeMin(Rappture::yaxis);
        dz = xfield.rangeMax(Rappture::zaxis) - xfield.rangeMin(Rappture::zaxis);
        double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

	printf("dx:%lf dy:%lf dz:%lf dmin:%lf\n", dx, dy, dz, dmin);

        nx = (int)ceil(dx/dmin);
        ny = (int)ceil(dy/dmin);
        nz = (int)ceil(dz/dmin);

#ifndef NV40
        // must be an even power of 2 for older cards
        nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
        ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
        nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

        float *data = new float[4*nx*ny*nz];
        memset(data, 0, sizeof(float) * 4 * nx * ny * nz);

        std::cout << "generating " << nx << "x" << ny << "x" << nz << " = " << nx*ny*nz << " points" << std::endl;

        // generate the uniformly sampled data that we need for a volume
        double vmin = 1e21;
        double vmax = -1e21;
        double nzero_min = 0.0;
        int ngen = 0;
        for (int iz=0; iz < nz; iz++) {
            double zval = z0 + iz*dmin;
            for (int iy=0; iy < ny; iy++) {
                double yval = y0 + iy*dmin;
                for (int ix=0; ix < nx; ix++) {
                    double xval = x0 + ix*dmin;

                    vx = xfield.value(xval,yval,zval);
                    vy = yfield.value(xval,yval,zval);
                    vz = zfield.value(xval,yval,zval);

                    double vm;
                    vm = sqrt(vx*vx + vy*vy + vz*vz);
                    if (vm < vmin) {
                        vmin = vm;
                    } else if (vm > vmax) {
                        vmax = vm;
                    }
                    if ((vm != 0.0f) && (vm < nzero_min)) {
                        nzero_min = vm;
                    }
                    data[ngen++] = vm;
                    data[ngen++] = vx;
                    data[ngen++] = vy;
                    data[ngen++] = vz;
                }
            }
        }

        ngen = 0;

        // scale should be accounted.
        for (ngen=0; ngen < npts; ) {
	    data[ngen] = data[ngen] / vmax; ++ngen;
            data[ngen] = (data[ngen]/(2.0*vmax) + 0.5); ++ngen;
            data[ngen] = (data[ngen]/(2.0*vmax) + 0.5); ++ngen;
            data[ngen] = (data[ngen]/(2.0*vmax) + 0.5); ++ngen;
        }
        Volume *volPtr;
        volPtr = NanoVis::load_volume(index, nx, ny, nz, 4, data, vmin, vmax,
                    nzero_min);

        volPtr->xAxis.SetRange(x0, x0 + (nx * dx));
        volPtr->yAxis.SetRange(y0, y0 + (ny * dy));
        volPtr->zAxis.SetRange(z0, z0 + (nz * dz));
        volPtr->wAxis.SetRange(vmin, vmax);
        volPtr->update_pending = true;
        delete [] data;
    } else {
        std::cerr << "WARNING: data not found in stream" << std::endl;
    }
}


/* Load a 3D volume from a dx-format file
 */
Rappture::Outcome
load_volume_stream2(int index, std::iostream& fin)
{
    printf("load_volume_stream2\n");
    Rappture::Outcome result;

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    int isrect = 1;
    dx = dy = dz = 0.0;         // Suppress compiler warning.
    x0 = y0 = z0 = 0.0;		// May not have an origin line.
    do {
        fin.getline(line,sizeof(line)-1);
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
                isrect = 1;
            } else if (sscanf(start, "object %d class array type float rank 1 shape 3 items %d data follows", &dummy, &nxy) == 2) {
                isrect = 0;
                double xx, yy, zz;
                for (int i=0; i < nxy; i++) {
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &xx, &yy, &zz) == 3) {
                        xymesh.addNode( Rappture::Node2D(xx,yy) );
                    }
                }

                char fpts[128];
                sprintf(fpts, "/tmp/tmppts%d", getpid());
                char fcells[128];
                sprintf(fcells, "/tmp/tmpcells%d", getpid());

                std::ofstream ftmp(fpts);
                // save corners of bounding box first, to work around meshing
                // problems in voronoi utility
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                for (int i=0; i < nxy; i++) {
                    ftmp << xymesh.atNode(i).x() << " " << xymesh.atNode(i).y() << std::endl;
                }
                ftmp.close();

                char cmdstr[512];
                sprintf(cmdstr, "voronoi -t < %s > %s", fpts, fcells);
                if (system(cmdstr) == 0) {
                    int cx, cy, cz;
                    std::ifstream ftri(fcells);
                    while (!ftri.eof()) {
                        ftri.getline(line,sizeof(line)-1);
                        if (sscanf(line, "%d %d %d", &cx, &cy, &cz) == 3) {
                            if (cx >= 4 && cy >= 4 && cz >= 4) {
                                // skip first 4 boundary points
                                xymesh.addCell(cx-4, cy-4, cz-4);
                            }
                        }
                    }
                    ftri.close();
                } else {
                    return result.error("triangularization failed");
                }

                sprintf(cmdstr, "rm -f %s %s", fpts, fcells);
                system(cmdstr);
            } else if (sscanf(start, "object %d class regulararray count %d", &dummy, &nz) == 2) {
                // found z-grid
            } else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            } else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
        int count = 0;
                // found one of the delta lines
                if (ddx != 0.0) {
            dx = ddx;
            count++;
        }
        if (ddy != 0.0) {
            dy = ddy;
            count++;
        }
        if (ddz != 0.0) {
            dz = ddz;
            count++;
        }
        if (count > 1) {
            return result.error(
            "don't know how to handle multiple non-zero delta values");
        }
            } else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows", &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                } else if (!isrect && (npts != nxy*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, npts);
                    return result.error(mesg);
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                }
                break;
            }
        }
    } while (!fin.eof());

    // read data points
    if (!fin.eof()) {
        if (isrect) {
            double dval[6];
            int nread = 0;
            int ix = 0;
            int iy = 0;
            int iz = 0;
            float* data = new float[nx *  ny *  nz * 4];
            memset(data, 0, nx*ny*nz*4);
            double vmin = 1e21;
            double nzero_min = 1e21;
            double vmax = -1e21;


            while (!fin.eof() && nread < npts) {
                fin.getline(line,sizeof(line)-1);
                int n = sscanf(line, "%lg %lg %lg %lg %lg %lg", &dval[0], &dval[1], &dval[2], &dval[3], &dval[4], &dval[5]);

                for (int p=0; p < n; p++) {
                    int nindex = (iz*nx*ny + iy*nx + ix) * 4;
                    data[nindex] = dval[p];

                    if (dval[p] < vmin) {
                        vmin = dval[p];
                    } else if (dval[p] > vmax) {
                        vmax = dval[p];
                    }
                    if (dval[p] != 0.0f && dval[p] < nzero_min) {
                        nzero_min = dval[p];
                    }

                    nread++;
                    if (++iz >= nz) {
                        iz = 0;
                        if (++iy >= ny) {
                            iy = 0;
                            ++ix;
                        }
                    }
                }
            }

            // make sure that we read all of the expected points
            if (nread != nx*ny*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, nread);
                result.error(mesg);
                return result;
            }

            double dv = vmax - vmin;
            int count = nx*ny*nz;
            int ngen = 0;
            double v;
            if (dv == 0.0) {
                dv = 1.0;
            }

            for (int i = 0; i < count; ++i) {
                v = data[ngen];
                // scale all values [0-1], -1 => out of bounds
                //
                // INSOO
                v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                data[ngen] = v;
                ngen += 4;
            }

            computeSimpleGradient(data, nx, ny, nz);

            dx = nx;
            dy = ny;
            dz = nz;

            Volume *volPtr;
            volPtr = NanoVis::load_volume(index, nx, ny, nz, 4, data,
                      vmin, vmax, nzero_min);
            volPtr->xAxis.SetRange(x0, x0 + (nx * dx));
            volPtr->yAxis.SetRange(y0, y0 + (ny * dy));
            volPtr->zAxis.SetRange(z0, z0 + (nz * dz)); 
            volPtr->wAxis.SetRange(vmin, vmax);
	    volPtr->update_pending = true;
            delete [] data;

        } else {
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldPrism3D field(xymesh, zgrid);

            double dval;
            int nread = 0;
            int ixy = 0;
            int iz = 0;
            while (!fin.eof() && nread < npts) {
                fin >> dval;
                if (fin.fail()) {
                    char mesg[256];
                    sprintf(mesg,"after %d of %d points: can't read number", 
                            nread, npts);
                    return result.error(mesg);
                } else {
                    int nid = nxy*iz + ixy;
                    field.define(nid, dval);

                    nread++;
                    if (++iz >= nz) {
                        iz = 0;
                        ixy++;
                    }
                }
            }

            // make sure that we read all of the expected points
            if (nread != nxy*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, nread);
                return result.error(mesg);
            }

            // figure out a good mesh spacing
            int nsample = 30;
            x0 = field.rangeMin(Rappture::xaxis);
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            y0 = field.rangeMin(Rappture::yaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            z0 = field.rangeMin(Rappture::zaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);
#ifndef NV40
            // must be an even power of 2 for older cards
            nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
            ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
            nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif
            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) {
                dv = 1.0; 
            }
            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            double nzero_min = 0.0;
            for (iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        if (v != 0.0f && v < nzero_min)
                            {
                                nzero_min = v;
                            }
                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen] = v;

                        ngen += 4;
                    }
                }
            }

            // FIXME: This next section of code should be replaced by a
            // call to the computeSimpleGradient() function. There is a slight
            // difference in the code below and the aforementioned function
            // in that the commented out lines in the else statements are
            // different.
            //
            // Compute the gradient of this data.  BE CAREFUL: center
            // calculation on each node to avoid skew in either direction.
            ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int ix=0; ix < nx; ix++) {
                        // gradient in x-direction
                        //double valm1 = (ix == 0) ? 0.0 : data[ngen-4];
                        //double valp1 = (ix == nx-1) ? 0.0 : data[ngen+4];
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-4];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+4];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = valp1-valm1; // assume dx=1
                            //data[ngen+1] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            data[ngen+2] = valp1-valm1; // assume dy=1
                            //data[ngen+2] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            data[ngen+3] = valp1-valm1; // assume dz=1
                            //data[ngen+3] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1
                        }

                        ngen += 4;
                    }
                }
            }

            Volume *volPtr;
            volPtr = NanoVis::load_volume(index, nx, ny, nz, 4, data,
                        field.valueMin(), field.valueMax(), nzero_min);
            volPtr->xAxis.SetRange(field.rangeMin(Rappture::xaxis),
                   field.rangeMax(Rappture::xaxis));
            volPtr->yAxis.SetRange(field.rangeMin(Rappture::yaxis),
                   field.rangeMax(Rappture::yaxis));
            volPtr->zAxis.SetRange(field.rangeMin(Rappture::zaxis),
                   field.rangeMax(Rappture::zaxis));
            volPtr->wAxis.SetRange(field.valueMin(), field.valueMax());
            volPtr->update_pending = true;
            delete [] data;
        }
    } else {
        return result.error("data not found in stream");
    }

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    NanoVis::volume[index]->move(Vector3(dx0, dy0, dz0));

    return result;
}

Rappture::Outcome
load_volume_stream(int index, std::iostream& fin)
{
    printf("load_volume_stream\n");
    Rappture::Outcome result;

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    int isrect = 1;

    dx = dy = dz = 0.0;         // Suppress compiler warning.
    x0 = y0 = z0 = 0.0;		// May not have an origin line.
    while (!fin.eof()) {
        fin.getline(line, sizeof(line) - 1);
        if (fin.fail()) {
            return result.error("error in data stream");
        }
        for (start=line; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
                isrect = 1;
            } else if (sscanf(start, "object %d class array type float rank 1 shape 3 items %d data follows", &dummy, &nxy) == 2) {
                isrect = 0;

                double xx, yy, zz;
                for (int i=0; i < nxy; i++) {
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &xx, &yy, &zz) == 3) {
                        xymesh.addNode( Rappture::Node2D(xx,yy) );
                    }
                }

                char fpts[128];
                sprintf(fpts, "/tmp/tmppts%d", getpid());
                char fcells[128];
                sprintf(fcells, "/tmp/tmpcells%d", getpid());

                std::ofstream ftmp(fpts);
                // save corners of bounding box first, to work around meshing
                // problems in voronoi utility
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                for (int i=0; i < nxy; i++) {
                    ftmp << xymesh.atNode(i).x() << " " << xymesh.atNode(i).y() << std::endl;

                }
                ftmp.close();

                char cmdstr[512];
                sprintf(cmdstr, "voronoi -t < %s > %s", fpts, fcells);
                if (system(cmdstr) == 0) {
                    int cx, cy, cz;
                    std::ifstream ftri(fcells);
                    while (!ftri.eof()) {
                        ftri.getline(line,sizeof(line)-1);
                        if (sscanf(line, "%d %d %d", &cx, &cy, &cz) == 3) {
                            if (cx >= 4 && cy >= 4 && cz >= 4) {
                                // skip first 4 boundary points
                                xymesh.addCell(cx-4, cy-4, cz-4);
                            }
                        }
                    }
                    ftri.close();
                } else {
                    return result.error("triangularization failed");
                }

                sprintf(cmdstr, "rm -f %s %s", fpts, fcells);
                system(cmdstr);
            } else if (sscanf(start, "object %d class regulararray count %d", &dummy, &nz) == 2) {
                // found z-grid
            } else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            } else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                if (ddx != 0.0) { dx = ddx; }
                else if (ddy != 0.0) { dy = ddy; }
                else if (ddz != 0.0) { dz = ddz; }
            } else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows", &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                } else if (!isrect && (npts != nxy*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, npts);
                    return result.error(mesg);
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                }
                break;
            }
        }
    }

    // read data points
    if (!fin.eof()) {
        if (isrect) {
            Rappture::Mesh1D xgrid(x0, x0+nx*dx, nx);
            Rappture::Mesh1D ygrid(y0, y0+ny*dy, ny);
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldRect3D field(xgrid, ygrid, zgrid);

            double dval[6];
            int nread = 0;
            int ix = 0;
            int iy = 0;
            int iz = 0;
            while (!fin.eof() && nread < npts) {
                fin.getline(line,sizeof(line)-1);
                if (fin.fail()) {
                    return result.error("error reading data points");
                }
                int n = sscanf(line, "%lg %lg %lg %lg %lg %lg", &dval[0], &dval[1], &dval[2], &dval[3], &dval[4], &dval[5]);

                for (int p=0; p < n; p++) {
                    int nindex = iz*nx*ny + iy*nx + ix;
                    field.define(nindex, dval[p]);
                    fprintf(stderr,"nindex = %i\tdval[%i] = %lg\n", nindex, p,
                        dval[p]);
                    fflush(stderr);
                    nread++;
                    if (++iz >= nz) {
                        iz = 0;
                        if (++iy >= ny) {
                            iy = 0;
                            ++ix;
                        }
                    }
                }
            }

            // make sure that we read all of the expected points
            if (nread != nx*ny*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, nread);
                result.error(mesg);
                return result;
            }

            // figure out a good mesh spacing
            int nsample = 30;
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);

#ifndef NV40
            // must be an even power of 2 for older cards
            nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
            ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
            nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

//#define _SOBEL
#ifdef _SOBEL_
            const int step = 1;
            float *cdata = new float[nx*ny*nz * step];
            int ngen = 0;
            double nzero_min = 0.0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        if (v != 0.0f && v < nzero_min) {
                            nzero_min = v;
                        }

                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : v;

                        cdata[ngen] = v;
                        ngen += step;
                    }
                }
            }

            float* data = computeGradient(cdata, nx, ny, nz, field.valueMin(),
                                          field.valueMax());
#else
            double vmin = field.valueMin();
            double vmax = field.valueMax();
            double nzero_min = 0;
            float *data = new float[nx*ny*nz * 4];
            double dv = vmax - vmin;
            int ngen = 0;
            if (dv == 0.0)  dv = 1.0; 

            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;

                        data[ngen] = v;
                        ngen += 4;
                    }
                }
            }

            computeSimpleGradient(data, nx, ny, nz);
#endif

            for (int i=0; i<nx*ny*nz; i++) {
                fprintf(stderr,"enddata[%i] = %lg\n",i,data[i]);
                fflush(stderr);
            }

            fprintf(stdout,"End Data Stats index = %i\n",index);
            fprintf(stdout,"nx = %i ny = %i nz = %i\n",nx,ny,nz);
            fprintf(stdout,"dx = %lg dy = %lg dz = %lg\n",dx,dy,dz);
            fprintf(stdout,"dataMin = %lg\tdataMax = %lg\tnzero_min = %lg\n", field.valueMin(),field.valueMax(),nzero_min);
            fflush(stdout);

            Volume *volPtr;
            volPtr = NanoVis::load_volume(index, nx, ny, nz, 4, data,
            field.valueMin(), field.valueMax(), nzero_min);
            volPtr->xAxis.SetRange(field.rangeMin(Rappture::xaxis),
                   field.rangeMax(Rappture::xaxis));
            volPtr->yAxis.SetRange(field.rangeMin(Rappture::yaxis),
                   field.rangeMax(Rappture::yaxis));
            volPtr->zAxis.SetRange(field.rangeMin(Rappture::zaxis),
                   field.rangeMax(Rappture::zaxis));
            volPtr->wAxis.SetRange(field.valueMin(), field.valueMax());
            volPtr->update_pending = true;
            // TBD..
            // POINTSET
            /*
              PointSet* pset = new PointSet();
              pset->initialize(volume[index], (float*) data);
              pset->setVisible(true);
              NanoVis::pointSet.push_back(pset);
              updateColor(pset);
              NanoVis::volume[index]->pointsetIndex = NanoVis::pointSet.size() - 1;
            */

            delete [] data;

        } else {
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldPrism3D field(xymesh, zgrid);

            double dval;
            int nread = 0;
            int ixy = 0;
            int iz = 0;
            while (!fin.eof() && nread < npts) {
                fin >> dval;
                if (fin.fail()) {
                    char mesg[256];
                    sprintf(mesg,"after %d of %d points: can't read number", 
                            nread, npts);
                    return result.error(mesg);
                } else {
                    int nid = nxy*iz + ixy;
                    field.define(nid, dval);

                    nread++;
                    if (++iz >= nz) {
                        iz = 0;
                        ixy++;
                    }
                }
            }

            // make sure that we read all of the expected points
            if (nread != nxy*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, nread);
                return result.error(mesg);
            }

            // figure out a good mesh spacing
            int nsample = 30;
            x0 = field.rangeMin(Rappture::xaxis);
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            y0 = field.rangeMin(Rappture::yaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            z0 = field.rangeMin(Rappture::zaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);
#ifndef NV40
            // must be an even power of 2 for older cards
            nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
            ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
            nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif
            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            double nzero_min = 0.0;
            for (iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        if (v != 0.0f && v < nzero_min)
                            {
                                nzero_min = v;
                            }
                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen] = v;

                        ngen += 4;
                    }
                }
            }

            // Compute the gradient of this data.  BE CAREFUL: center
            // calculation on each node to avoid skew in either direction.
            ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int ix=0; ix < nx; ix++) {
                        // gradient in x-direction
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-1];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+1];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = valp1-valm1; // assume dx=1
                            //data[ngen+1] = ((valp1-valm1) + 1) *  0.5; // assume dx=1 (ISO)
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            data[ngen+2] = valp1-valm1; // assume dy=1
                            //data[ngen+2] = ((valp1-valm1) + 1) *  0.5; // assume dy=1 (ISO)
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            data[ngen+3] = valp1-valm1; // assume dz=1
                            //data[ngen+3] = ((valp1-valm1) + 1) *  0.5; // assume dz=1 (ISO)
                        }

                        ngen += 4;
                    }
                }
            }

            Volume *volPtr;
            volPtr = NanoVis::load_volume(index, nx, ny, nz, 4, data,
            field.valueMin(), field.valueMax(), nzero_min);
            volPtr->xAxis.SetRange(field.rangeMin(Rappture::xaxis),
                   field.rangeMax(Rappture::xaxis));
            volPtr->yAxis.SetRange(field.rangeMin(Rappture::yaxis),
                   field.rangeMax(Rappture::yaxis));
            volPtr->zAxis.SetRange(field.rangeMin(Rappture::zaxis),
                   field.rangeMax(Rappture::zaxis));
            volPtr->wAxis.SetRange(field.valueMin(), field.valueMax());
            volPtr->update_pending = true;
            // TBD..
            // POINTSET
            /*
              PointSet* pset = new PointSet();
              pset->initialize(volume[index], (float*) data);
              pset->setVisible(true);
              NanoVis::pointSet.push_back(pset);
              updateColor(pset);
              NanoVis::volume[index]->pointsetIndex = NanoVis::pointSet.size() - 1;
            */


            delete [] data;
        }
    } else {
        return result.error("data not found in stream");
    }

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    NanoVis::volume[index]->move(Vector3(dx0, dy0, dz0));
    return result;
}

Rappture::Outcome
load_volume_stream_insoo(int index, std::iostream& fin)
{
    printf("load_volume_stream\n");
    Rappture::Outcome result;

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    int isrect = 1;

    dx = dy = dz = 0.0;         // Suppress compiler warning.
    x0 = y0 = z0 = 0.0;		// May not have an origin line.
    while (!fin.eof()) {
        fin.getline(line, sizeof(line) - 1);
        if (fin.fail()) {
            return result.error("error in data stream");
        }
        for (start=line; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
                isrect = 1;
            } else if (sscanf(start, "object %d class array type float rank 1 shape 3 items %d data follows", &dummy, &nxy) == 2) {
                isrect = 0;

                double xx, yy, zz;
                for (int i=0; i < nxy; i++) {
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &xx, &yy, &zz) == 3) {
                        xymesh.addNode( Rappture::Node2D(xx,yy) );
                    }
                }

                char fpts[128];
                sprintf(fpts, "/tmp/tmppts%d", getpid());
                char fcells[128];
                sprintf(fcells, "/tmp/tmpcells%d", getpid());

                std::ofstream ftmp(fpts);
                // save corners of bounding box first, to work around meshing
                // problems in voronoi utility
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                for (int i=0; i < nxy; i++) {
                    ftmp << xymesh.atNode(i).x() << " " << xymesh.atNode(i).y() << std::endl;

                }
                ftmp.close();

                char cmdstr[512];
                sprintf(cmdstr, "voronoi -t < %s > %s", fpts, fcells);
                if (system(cmdstr) == 0) {
                    int cx, cy, cz;
                    std::ifstream ftri(fcells);
                    while (!ftri.eof()) {
                        ftri.getline(line,sizeof(line)-1);
                        if (sscanf(line, "%d %d %d", &cx, &cy, &cz) == 3) {
                            if (cx >= 4 && cy >= 4 && cz >= 4) {
                                // skip first 4 boundary points
                                xymesh.addCell(cx-4, cy-4, cz-4);
                            }
                        }
                    }
                    ftri.close();
                } else {
                    return result.error("triangularization failed");
                }

                sprintf(cmdstr, "rm -f %s %s", fpts, fcells);
                system(cmdstr);
            } else if (sscanf(start, "object %d class regulararray count %d", &dummy, &nz) == 2) {
                // found z-grid
            } else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            } else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                if (ddx != 0.0) { dx = ddx; }
                else if (ddy != 0.0) { dy = ddy; }
                else if (ddz != 0.0) { dz = ddz; }
            } else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows", &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                } else if (!isrect && (npts != nxy*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, npts);
                    return result.error(mesg);
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                }
                break;
            }
        }
    }

    // read data points
    if (!fin.eof()) {
        if (isrect) {
            Rappture::Mesh1D xgrid(x0, x0+nx*dx, nx);
            Rappture::Mesh1D ygrid(y0, y0+ny*dy, ny);
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldRect3D field(xgrid, ygrid, zgrid);

            double dval[6];
            int nread = 0;
            int ix = 0;
            int iy = 0;
            int iz = 0;
            while (!fin.eof() && nread < npts) {
                fin.getline(line,sizeof(line)-1);
                if (fin.fail()) {
                    return result.error("error reading data points");
                }
                int n = sscanf(line, "%lg %lg %lg %lg %lg %lg", &dval[0], &dval[1], &dval[2], &dval[3], &dval[4], &dval[5]);

                for (int p=0; p < n; p++) {
                    int nindex = iz*nx*ny + iy*nx + ix;
                    field.define(nindex, dval[p]);
                    fprintf(stderr,"nindex = %i\tdval[%i] = %lg\n", nindex, p,
                        dval[p]);
                    fflush(stderr);
                    nread++;
                    if (++iz >= nz) {
                        iz = 0;
                        if (++iy >= ny) {
                            iy = 0;
                            ++ix;
                        }
                    }
                }
            }

            // make sure that we read all of the expected points
            if (nread != nx*ny*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, nread);
                result.error(mesg);
                return result;
            }

            // figure out a good mesh spacing
            int nsample = 30;
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);

#ifndef NV40
            // must be an even power of 2 for older cards
            nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
            ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
            nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }

            int ngen = 0;
            double nzero_min = 0.0;
            for (iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        if (v != 0.0f && v < nzero_min)
                            {
                                nzero_min = v;
                            }
                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen] = v;

                        ngen += 4;
                    }
                }
            }

            // Compute the gradient of this data.  BE CAREFUL: center
            // calculation on each node to avoid skew in either direction.
            ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int ix=0; ix < nx; ix++) {
                        // gradient in x-direction
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-1];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+1];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = valp1-valm1; // assume dx=1
                            //data[ngen+1] = ((valp1-valm1) + 1) *  0.5; // assume dx=1 (ISO)
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            data[ngen+2] = valp1-valm1; // assume dy=1
                            //data[ngen+2] = ((valp1-valm1) + 1) *  0.5; // assume dy=1 (ISO)
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            data[ngen+3] = valp1-valm1; // assume dz=1
                            //data[ngen+3] = ((valp1-valm1) + 1) *  0.5; // assume dz=1 (ISO)
                        }

                        ngen += 4;
                    }
                }
            }

/*
            float *cdata = new float[nx*ny*nz];
            int ngen = 0;
            double nzero_min = 0.0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        if (v != 0.0f && v < nzero_min) {
                            nzero_min = v;
                        }

                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : v;

                        cdata[ngen] = v;
                        ++ngen;
                    }
                }
            }

            float* data = computeGradient(cdata, nx, ny, nz, field.valueMin(),
                                          field.valueMax());

            for (int i=0; i<nx*ny*nz; i++) {
                fprintf(stderr,"enddata[%i] = %lg\n",i,data[i]);
                fflush(stderr);
            }

            fprintf(stdout,"End Data Stats index = %i\n",index);
            fprintf(stdout,"nx = %i ny = %i nz = %i\n",nx,ny,nz);
            fprintf(stdout,"dx = %lg dy = %lg dz = %lg\n",dx,dy,dz);
            fprintf(stdout,"dataMin = %lg\tdataMax = %lg\tnzero_min = %lg\n", field.valueMin(),field.valueMax(),nzero_min);
            fflush(stdout);
            */

            Volume *volPtr;
            volPtr = NanoVis::load_volume(index, nx, ny, nz, 4, data,
            field.valueMin(), field.valueMax(), nzero_min);
            volPtr->xAxis.SetRange(field.rangeMin(Rappture::xaxis),
                   field.rangeMax(Rappture::xaxis));
            volPtr->yAxis.SetRange(field.rangeMin(Rappture::yaxis),
                   field.rangeMax(Rappture::yaxis));
            volPtr->zAxis.SetRange(field.rangeMin(Rappture::zaxis),
                   field.rangeMax(Rappture::zaxis));
            volPtr->wAxis.SetRange(field.valueMin(), field.valueMax());
            volPtr->update_pending = true;
            // TBD..
            // POINTSET
            /*
              PointSet* pset = new PointSet();
              pset->initialize(volume[index], (float*) data);
              pset->setVisible(true);
              NanoVis::pointSet.push_back(pset);
              updateColor(pset);
              NanoVis::volume[index]->pointsetIndex = NanoVis::pointSet.size() - 1;
            */

            delete [] data;

        } else {
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldPrism3D field(xymesh, zgrid);

            double dval;
            int nread = 0;
            int ixy = 0;
            int iz = 0;
            while (!fin.eof() && nread < npts) {
                fin >> dval;
                if (fin.fail()) {
                    char mesg[256];
                    sprintf(mesg,"after %d of %d points: can't read number", 
                            nread, npts);
                    return result.error(mesg);
                } else {
                    int nid = nxy*iz + ixy;
                    field.define(nid, dval);

                    nread++;
                    if (++iz >= nz) {
                        iz = 0;
                        ixy++;
                    }
                }
            }

            // make sure that we read all of the expected points
            if (nread != nxy*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, nread);
                return result.error(mesg);
            }

            // figure out a good mesh spacing
            int nsample = 30;
            x0 = field.rangeMin(Rappture::xaxis);
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            y0 = field.rangeMin(Rappture::yaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            z0 = field.rangeMin(Rappture::zaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);
#ifndef NV40
            // must be an even power of 2 for older cards
            nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
            ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
            nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif
            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            double nzero_min = 0.0;
            for (iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        if (v != 0.0f && v < nzero_min)
                            {
                                nzero_min = v;
                            }
                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen] = v;

                        ngen += 4;
                    }
                }
            }

            // Compute the gradient of this data.  BE CAREFUL: center
            // calculation on each node to avoid skew in either direction.
            ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int ix=0; ix < nx; ix++) {
                        // gradient in x-direction
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-1];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+1];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = valp1-valm1; // assume dx=1
                            //data[ngen+1] = ((valp1-valm1) + 1) *  0.5; // assume dx=1 (ISO)
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            data[ngen+2] = valp1-valm1; // assume dy=1
                            //data[ngen+2] = ((valp1-valm1) + 1) *  0.5; // assume dy=1 (ISO)
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            data[ngen+3] = valp1-valm1; // assume dz=1
                            //data[ngen+3] = ((valp1-valm1) + 1) *  0.5; // assume dz=1 (ISO)
                        }

                        ngen += 4;
                    }
                }
            }

            Volume *volPtr;
            volPtr = NanoVis::load_volume(index, nx, ny, nz, 4, data,
            field.valueMin(), field.valueMax(), nzero_min);
            volPtr->xAxis.SetRange(field.rangeMin(Rappture::xaxis),
                   field.rangeMax(Rappture::xaxis));
            volPtr->yAxis.SetRange(field.rangeMin(Rappture::yaxis),
                   field.rangeMax(Rappture::yaxis));
            volPtr->zAxis.SetRange(field.rangeMin(Rappture::zaxis),
                   field.rangeMax(Rappture::zaxis));
            volPtr->wAxis.SetRange(field.valueMin(), field.valueMax());
            volPtr->update_pending = true;
            // TBD..
            // POINTSET
            /*
              PointSet* pset = new PointSet();
              pset->initialize(volume[index], (float*) data);
              pset->setVisible(true);
              NanoVis::pointSet.push_back(pset);
              updateColor(pset);
              NanoVis::volume[index]->pointsetIndex = NanoVis::pointSet.size() - 1;
            */


            delete [] data;
        }
    } else {
        return result.error("data not found in stream");
    }

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    NanoVis::volume[index]->move(Vector3(dx0, dy0, dz0));
    return result;
}
