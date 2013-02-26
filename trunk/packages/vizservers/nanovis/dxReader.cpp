 /* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <float.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <RpOutcome.h>
#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>

// common dx functions
#include "dxReaderCommon.h"

#include "config.h"
#include "nanovis.h"
#include "Unirect.h"
#include "Volume.h"
#include "ZincBlendeVolume.h"
#include "NvZincBlendeReconstructor.h"
#ifdef USE_POINTSET_RENDERER
#include "PointSet.h"
#endif

/**
 * \brief Load a 3D volume from a dx-format file
 *
 * In DX format:
 *  rank 0 means scalars,
 *  rank 1 means vectors,
 *  rank 2 means matrices,
 *  rank 3 means tensors
 * 
 *  For rank 1, shape is a single number equal to the number of dimensions.
 *  e.g. rank 1 shape 3 means a 3-component vector field
 */
Volume *
load_volume_stream(Rappture::Outcome& result, const char *tag,
                   std::iostream& fin)
{
    TRACE("load_volume_stream %s\n", tag);

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    // origin
    double x0, y0, z0;
    // deltas (cell dimensions)
    double dx, dy, dz;
    // length of volume edges
    double lx, ly, lz;
    // temps for delta values
    double ddx, ddy, ddz;
    char line[128], type[128], *start;

    int isrect = 1;

    dx = dy = dz = 0.0;
    x0 = y0 = z0 = 0.0; // May not have an origin line.
    nx = ny = nz = npts = nxy = 0;
    while (!fin.eof()) {
        fin.getline(line, sizeof(line) - 1);
        if (fin.fail()) {
            result.error("error in data stream");
            return NULL;
        }
        for (start = line; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d",
                       &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
                isrect = 1;
            } else if (sscanf(start, "object %d class array type float rank 1 shape 3 items %d data follows",
                              &dummy, &nxy) == 2) {
                isrect = 0;

                double xx, yy, zz;
                for (int i = 0; i < nxy; i++) {
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &xx, &yy, &zz) == 3) {
                        xymesh.addNode(Rappture::Node2D(xx, yy));
                    }
                }

                char fpts[128];
                sprintf(fpts, "/tmp/tmppts%d", getpid());
                char fcells[128];
                sprintf(fcells, "/tmp/tmpcells%d", getpid());

                std::ofstream ftmp(fpts);
                // save corners of bounding box first, to work around meshing
                // problems in voronoi utility
                int numBoundaryPoints = 4;

                // Bump out the bounds by an epsilon to avoid problem when
                // corner points are already nodes
                double XEPS = (xymesh.rangeMax(Rappture::xaxis) - 
                               xymesh.rangeMin(Rappture::xaxis)) / 10.0f;

                double YEPS = (xymesh.rangeMax(Rappture::yaxis) - 
                               xymesh.rangeMin(Rappture::yaxis)) / 10.0f;

                ftmp << xymesh.rangeMin(Rappture::xaxis) - XEPS << " "
                     << xymesh.rangeMin(Rappture::yaxis) - YEPS << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) + XEPS << " "
                     << xymesh.rangeMin(Rappture::yaxis) - YEPS << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) + XEPS << " "
                     << xymesh.rangeMax(Rappture::yaxis) + YEPS << std::endl;
                ftmp << xymesh.rangeMin(Rappture::xaxis) - XEPS << " "
                     << xymesh.rangeMax(Rappture::yaxis) + YEPS << std::endl;

                for (int i = 0; i < nxy; i++) {
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
                            if (cx >= numBoundaryPoints &&
                                cy >= numBoundaryPoints &&
                                cz >= numBoundaryPoints) {
                                // skip boundary points we added
                                xymesh.addCell(cx - numBoundaryPoints,
                                               cy - numBoundaryPoints,
                                               cz - numBoundaryPoints);
                            }
                        }
                    }
                    ftri.close();
                } else {
                    result.error("triangularization failed");
                    return NULL;
                }
                unlink(fpts);
                unlink(fcells);
            } else if (sscanf(start, "object %d class regulararray count %d", &dummy, &nz) == 2) {
                // found z-grid
            } else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            } else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                int count = 0;
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
                    result.addError("don't know how to handle multiple non-zero"
                                    " delta values");
                    return NULL;
                }
            } else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows",
                              &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    result.addError("inconsistent data: expected %d points"
                                    " but found %d points", nx*ny*nz, npts);
                    return NULL;
                } else if (!isrect && (npts != nxy*nz)) {
                    result.addError("inconsistent data: expected %d points"
                                    " but found %d points", nxy*nz, npts);
                    return NULL;
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows",
                              &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    result.addError("inconsistent data: expected %d points"
                                    " but found %d points", nx*ny*nz, npts);
                    return NULL;
                }
                break;
            }
        }
    }

    TRACE("found nx=%d ny=%d nxy=%d nz=%d\ndx=%f dy=%f dz=%f\nx0=%f y0=%f z0=%f\n", 
          nx, ny, nxy, nz, dx, dy, dz, x0, y0, z0);

    lx = (nx - 1) * dx;
    ly = (ny - 1) * dy;
    lz = (nz - 1) * dz;

    // read data points
    if (fin.eof()) {
        result.error("data not found in stream");
        return NULL;
    }
    Volume *volPtr = NULL;
    float *data = NULL;
    double vmin = DBL_MAX;
    double nzero_min = DBL_MAX;
    double vmax = -DBL_MAX;
    if (isrect) {
#ifdef DOWNSAMPLE_DATA
        Rappture::Mesh1D xgrid(x0, x0 + lx, nx);
        Rappture::Mesh1D ygrid(y0, y0 + ly, ny);
        Rappture::Mesh1D zgrid(z0, z0 + lz, nz);
        Rappture::FieldRect3D field(xgrid, ygrid, zgrid);
#else // !DOWNSAMPLE_DATA
        data = new float[nx *  ny *  nz * 4];
        memset(data, 0, nx*ny*nz*4);
#endif // DOWNSAMPLE_DATA
        double dval[6];
        int nread = 0;
        int ix = 0;
        int iy = 0;
        int iz = 0;

        while (!fin.eof() && nread < npts) {
            fin.getline(line,sizeof(line)-1);
            if (fin.fail()) {
                result.addError("error reading data points");
                return NULL;
            }
            int n = sscanf(line, "%lg %lg %lg %lg %lg %lg",
                           &dval[0], &dval[1], &dval[2], &dval[3], &dval[4], &dval[5]);
            for (int p = 0; p < n; p++) {
#ifdef notdef
                if (isnan(dval[p])) {
                    TRACE("Found NAN in input at %d,%d,%d\n", ix, iy, iz);
                }
#endif
#ifdef DOWNSAMPLE_DATA
                int nindex = iz*nx*ny + iy*nx + ix;
                field.define(nindex, dval[p]);
#else // !DOWNSAMPLE_DATA
                int nindex = (iz*nx*ny + iy*nx + ix) * 4;
                data[nindex] = dval[p];

                if (dval[p] < vmin) {
                    vmin = dval[p];
                } else if (dval[p] > vmax) {
                    vmax = dval[p];
                }
                if (dval[p] != 0.0 && dval[p] < nzero_min) {
                    nzero_min = dval[p];
                }
#endif // DOWNSAMPLE_DATA
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
            result.addError("inconsistent data: expected %d points"
                            " but found %d points", nx*ny*nz, nread);
            return NULL;
        }

#ifndef DOWNSAMPLE_DATA
        double dv = vmax - vmin;
        if (dv == 0.0) {
            dv = 1.0;
        }

        int ngen = 0;
        const int step = 4;
        for (int i = 0; i < nx*ny*nz; ++i) {
            double v = data[ngen];
            // scale all values [0-1], -1 => out of bounds
            v = (isnan(v)) ? -1.0 : (v - vmin)/dv;

            data[ngen] = v;
            ngen += step;
        }

        computeSimpleGradient(data, nx, ny, nz,
                              dx, dy, dz);
#else // DOWNSAMPLE_DATA
        // figure out a good mesh spacing
        int nsample = 30;
        double dmin = pow((lx*ly*lz)/((nsample-1)*(nsample-1)*(nsample-1)), 0.333);

        nx = (int)ceil(lx/dmin) + 1;
        ny = (int)ceil(ly/dmin) + 1;
        nz = (int)ceil(lz/dmin) + 1;

#ifndef HAVE_NPOT_TEXTURES
        // must be an even power of 2 for older cards
        nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
        ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
        nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

        dx = lx /(double)(nx - 1);
        dy = ly /(double)(ny - 1);
        dz = lz /(double)(nz - 1);

        vmin = field.valueMin();
        vmax = field.valueMax();
        nzero_min = DBL_MAX;
        double dv = vmax - vmin;
        if (dv == 0.0) {
            dv = 1.0;
        }

        int ngen = 0;
#ifdef FILTER_GRADIENTS
        // Sobel filter expects a single float at
        // each node
        const int step = 1;
        float *cdata = new float[nx*ny*nz * step];
#else // !FILTER_GRADIENTS
        // Leave 3 floats of space for gradients
        // to be filled in by computeSimpleGradient()
        const int step = 4;
        data = new float[nx*ny*nz * step];
#endif // FILTER_GRADIENTS

        for (int iz = 0; iz < nz; iz++) {
            double zval = z0 + iz*dz;
            for (int iy = 0; iy < ny; iy++) {
                double yval = y0 + iy*dy;
                for (int ix = 0; ix < nx; ix++) {
                    double xval = x0 + ix*dx;
                    double v = field.value(xval, yval, zval);
#ifdef notdef
                    if (isnan(v)) {
                        TRACE("NAN at %d,%d,%d: (%g, %g, %g)", ix, iy, iz, xval, yval, zval);
                    }
#endif
                    if (v != 0.0 && v < nzero_min) {
                        nzero_min = v;
                    }
#ifdef FILTER_GRADIENTS
                    // NOT normalized, -1 => out of bounds
                    v = (isnan(v)) ? -1.0 : v;
                    cdata[ngen] = v;
#else // !FILTER_GRADIENTS
                    // scale all values [0-1], -1 => out of bounds
                    v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                    data[ngen] = v;
#endif // FILTER_GRADIENTS
                    ngen += step;
                }
            }
        }
#ifdef FILTER_GRADIENTS
        // computeGradient returns a new array with gradients
        // filled in, so data is now 4 floats per node
        data = computeGradient(cdata, nx, ny, nz,
                               dx, dy, dz,
                               vmin, vmax);
        delete [] cdata;
#else // !FILTER_GRADIENTS
        computeSimpleGradient(data, nx, ny, nz,
                              dx, dy, dz);
#endif // FILTER_GRADIENTS

#endif // DOWNSAMPLE_DATA
    } else {
        Rappture::Mesh1D zgrid(z0, z0 + (nz-1)*dz, nz);
        Rappture::FieldPrism3D field(xymesh, zgrid);

        double dval;
        int nread = 0;
        int ixy = 0;
        int iz = 0;
        while (!fin.eof() && nread < npts) {
            fin >> dval;
            if (fin.fail()) {
                result.addError("after %d of %d points: can't read number", 
                                nread, npts);
                return NULL;
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
            result.addError("inconsistent data: expected %d points"
                            " but found %d points", nxy*nz, nread);
            return NULL;
        }

        x0 = field.rangeMin(Rappture::xaxis);
        lx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
        y0 = field.rangeMin(Rappture::yaxis);
        ly = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
        z0 = field.rangeMin(Rappture::zaxis);
        lz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);

        // figure out a good mesh spacing
        int nsample = 30;
        double dmin = pow((lx*ly*lz)/((nsample-1)*(nsample-1)*(nsample-1)), 0.333);

        nx = (int)ceil(lx/dmin) + 1;
        ny = (int)ceil(ly/dmin) + 1;
        nz = (int)ceil(lz/dmin) + 1;
#ifndef HAVE_NPOT_TEXTURES
        // must be an even power of 2 for older cards
        nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
        ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
        nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

        dx = lx /(double)(nx - 1);
        dy = ly /(double)(ny - 1);
        dz = lz /(double)(nz - 1);

        vmin = field.valueMin();
        vmax = field.valueMax();
        nzero_min = DBL_MAX;
        double dv = vmax - vmin;
        if (dv == 0.0) {
            dv = 1.0;
        }

        data = new float[4*nx*ny*nz];
        // generate the uniformly sampled data that we need for a volume
        int ngen = 0;
        for (int iz = 0; iz < nz; iz++) {
            double zval = z0 + iz*dz;
            for (int iy = 0; iy < ny; iy++) {
                double yval = y0 + iy*dy;
                for (int ix = 0; ix < nx; ix++) {
                    double xval = x0 + ix*dx;
                    double v = field.value(xval, yval, zval);

                    if (v != 0.0 && v < nzero_min) {
                        nzero_min = v;
                    }
                    // scale all values [0-1], -1 => out of bounds
                    v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                    data[ngen] = v;

                    ngen += 4;
                }
            }
        }

        computeSimpleGradient(data, nx, ny, nz,
                              dx, dy, dz);
    }

    TRACE("nx = %i ny = %i nz = %i\n", nx, ny, nz);
    TRACE("x0 = %lg y0 = %lg z0 = %lg\n", x0, y0, z0);
    TRACE("lx = %lg ly = %lg lz = %lg\n", lx, ly, lz);
    TRACE("dx = %lg dy = %lg dz = %lg\n", dx, dy, dz);
    TRACE("dataMin = %lg dataMax = %lg nzero_min = %lg\n",
          vmin, vmax, nzero_min);

    volPtr = NanoVis::loadVolume(tag, nx, ny, nz, 4, data,
                                 vmin, vmax, nzero_min);
    volPtr->xAxis.setRange(x0, x0 + lx);
    volPtr->yAxis.setRange(y0, y0 + ly);
    volPtr->zAxis.setRange(z0, z0 + lz);
    volPtr->updatePending = true;

    // TBD..
#if 0 && defined(USE_POINTSET_RENDERER)
    PointSet *pset = new PointSet();
    pset->initialize(volPtr, (float*)data);
    pset->setVisible(true);
    NanoVis::pointSet.push_back(pset);
    updateColor(pset);
    volPtr->pointsetIndex = NanoVis::pointSet.size() - 1;
#endif
    delete [] data;

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*ly/lx;
    float dz0 = -0.5*lz/lx;
    if (volPtr) {
        volPtr->location(Vector3(dx0, dy0, dz0));
        TRACE("Set volume location to %g %g %g\n", dx0, dy0, dz0);
    }
    return volPtr;
}
