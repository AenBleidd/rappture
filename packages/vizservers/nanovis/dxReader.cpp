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
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>

// common dx functions
#include "dxReaderCommon.h"

#include "nanovis.h"
#include "Unirect.h"
#include "ZincBlendeVolume.h"
#include "NvZincBlendeReconstructor.h"

/* Load a 3D volume from a dx-format file
 */
Volume *
load_volume_stream2(Rappture::Outcome &result, const char *tag, 
                    std::iostream& fin)
{
    TRACE("load_volume_stream2 %s\n", tag);
    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    int isrect = 1;
    dx = dy = dz = 0.0;         // Suppress compiler warning.
    x0 = y0 = z0 = 0.0;		// May not have an origin line.
    nx = ny = nz = npts = nxy = 0;
    do {
        fin.getline(line,sizeof(line)-1);
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", 
                       &dummy, &nx, &ny, &nz) == 4) {
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
                    result.error("triangularization failed");
                    return NULL;
                }
                unlink(fpts), unlink(fcells);
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
            result.addError("don't know how to handle multiple non-zero"
                            " delta values");
            return NULL;
        }
            } else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows", &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    result.addError("inconsistent data: expected %d points "
                                    " but found %d points", nx*ny*nz, npts);
                    return NULL;
                } else if (!isrect && (npts != nxy*nz)) {
                    result.addError("inconsistent data: expected %d points "
                                    " but found %d points", nxy*nz, npts);
                    return NULL;
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    result.addError("inconsistent data: expected %d points "
                                    " but found %d points", nx*ny*nz, npts);
                    return NULL;
                }
                break;
            }
        }
    } while (!fin.eof());

    TRACE("found nx=%d ny=%d, nz=%d, x0=%f, y0=%f, z0=%f\n", 
           nx, ny, nz, x0, y0, z0);
    // read data points
    if (fin.eof() && (npts > 0)) {
        result.addError("EOF found: expecting %d points", npts);
        return NULL;
    }
    Volume *volPtr = NULL;
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
            result.addError("inconsistent data: expected %d points "
                            " but found %d points", nx*ny*nz, nread);
            return NULL;
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
        
        volPtr = NanoVis::load_volume(tag, nx, ny, nz, 4, data,
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
            result.addError("inconsistent data: expected %d points "
                            "but found %d points", nxy*nz, nread);
            return NULL;
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
                    
                    if (v != 0.0f && v < nzero_min) {
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
        
        volPtr = NanoVis::load_volume(tag, nx, ny, nz, 4, data, 
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
    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    if (volPtr) {
        volPtr->location(Vector3(dx0, dy0, dz0));
        TRACE("volume moved\n");
    }
    return volPtr;
}

Volume *
load_volume_stream(Rappture::Outcome &result, const char *tag, 
                   std::iostream& fin)
{
    TRACE("load_volume_stream\n");

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
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
                if (ddx != 0.0) { dx = ddx; }
                else if (ddy != 0.0) { dy = ddy; }
                else if (ddz != 0.0) { dz = ddz; }
            } else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows", &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    result.addError("inconsistent data: expected %d points"
                                    " but found %d points", nx*ny*nz, npts);
                    return NULL;
                } else if (!isrect && (npts != nxy*nz)) {
                    result.addError("inconsistent data: expected %d points"
                                    " but found %d points", nx*ny*nz, npts);
                    return NULL;
                }
                break;
            } else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    result.addError("inconsistent data: expected %d points"
                                    " but found %d points", nx*ny*nz, npts);
                    return NULL;
                }
                break;
            }
        }
    }
    // read data points
    if (fin.eof()) {
        result.error("data not found in stream");
        return NULL;
    }
    Volume *volPtr = 0;
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
                result.addError("error reading data points");
                return NULL;
            }
            int n = sscanf(line, "%lg %lg %lg %lg %lg %lg", &dval[0], &dval[1], &dval[2], &dval[3], &dval[4], &dval[5]);
            
            for (int p=0; p < n; p++) {
                int nindex = iz*nx*ny + iy*nx + ix;
                field.define(nindex, dval[p]);
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
                            " but found %d points", nx*ny*nz, npts);
            return NULL;
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
        
#ifdef notdef
        for (int i=0; i<nx*ny*nz; i++) {
            TRACE("enddata[%i] = %lg\n",i,data[i]);
        }
#endif        
        TRACE("nx = %i ny = %i nz = %i\n",nx,ny,nz);
        TRACE("dx = %lg dy = %lg dz = %lg\n",dx,dy,dz);
        TRACE("dataMin = %lg\tdataMax = %lg\tnzero_min = %lg\n", 
               field.valueMin(),field.valueMax(),nzero_min);
        
        volPtr = NanoVis::load_volume(tag, nx, ny, nz, 4, data,
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
                            " but found %d points", nx*ny*nz, npts);
            return NULL;
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
                    
                    if (v != 0.0f && v < nzero_min) {
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
        
        volPtr = NanoVis::load_volume(tag, nx, ny, nz, 4, data,
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

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    if (volPtr) {
        volPtr->location(Vector3(dx0, dy0, dz0));
    }
    return volPtr;
}
