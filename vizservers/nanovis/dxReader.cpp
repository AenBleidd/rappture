
/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
 *
 *  dxReader.cpp 
 *	This module contains openDX readers for 2D and 3D volumes.
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

#include "Nv.h"

#include "nanovis.h"
#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"

//transfer function headers
#include "ZincBlendeVolume.h"
#include "NvZincBlendeReconstructor.h"
#include "GradientFilter.h"

//#define  _LOCAL_ZINC_TEST_
float* merge(float* scalar, float* gradient, int size)
{
	float* data = (float*) malloc(sizeof(float) * 4 * size);

	Vector3* g = (Vector3*) gradient;

	int ngen = 0, sindex = 0;
	for (sindex = 0; sindex <size; ++sindex)
	{
		data[ngen++] = scalar[sindex];
		data[ngen++] = g[sindex].x;
		data[ngen++] = g[sindex].y;
		data[ngen++] = g[sindex].z;
	}
	return data;
}

void normalizeScalar(float* fdata, int count, float min, float max)
{
    float v = max - min;
    if (v != 0.0f) 
    {
        for (int i = 0; i < count; ++i)
            fdata[i] = fdata[i] / v;
    }
}

float* computeGradient(float* fdata, int width, int height, int depth, float min, float max)
{
		float* gradients = (float *)malloc(width * height * depth * 3 * sizeof(float));
		float* tempGradients = (float *)malloc(width * height * depth * 3 * sizeof(float));
		int sizes[3] = { width, height, depth };
		computeGradients(tempGradients, fdata, sizes, DATRAW_FLOAT);
		filterGradients(tempGradients, sizes);
		quantizeGradients(tempGradients, gradients, sizes, DATRAW_FLOAT);
		normalizeScalar(fdata, width * height * depth, min, max);
		float* data = merge(fdata, gradients, width * height * depth);

        return data;
}

/* 
 * Load a 3D vector field from a dx-format file
 */
void
load_vector_stream(int index, std::istream& fin) 
{
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

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
                // found grid size
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
            else if (sscanf(start, "object %d class array type %s shape 3 rank 1 items %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                break;
            }
            else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
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

        nx = (int)ceil(dx/dmin);
        ny = (int)ceil(dy/dmin);
        nz = (int)ceil(dz/dmin);

#ifndef NV40
        // must be an even power of 2 for older cards
	    nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
	    ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
	    nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

        float *data = new float[3*nx*ny*nz];
        memset(data, 0, sizeof(float) * 3 * nx * ny * nz);

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

                    double vm = sqrt(vx*vx + vy*vy + vz*vz);

                    if (vm < vmin) { vmin = vm; }
                    if (vm > vmax) { vmax = vm; }
                    if (vm != 0.0f && vm < nzero_min)
                    {
                        nzero_min = vm;
                    }

                    data[ngen++] = vx;
                    data[ngen++] = vy;
                    data[ngen++] = vz;
                }
            }
        }

        ngen = 0;

        // scale should be accounted.
        for (ngen=0; ngen < npts; ngen++) 
        {
            data[ngen] = (data[ngen]/(2.0*vmax) + 0.5);
        }

        NanoVis::load_volume(index, nx, ny, nz, 3, data, vmin, vmax, nzero_min);

        delete [] data;
    } 
    else {
        std::cerr << "WARNING: data not found in stream" << std::endl;
    }
}


/* Load a 3D volume from a dx-format file
 */
Rappture::Outcome
load_volume_stream2(int index, std::iostream& fin) 
{
    Rappture::Outcome result;

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    int isrect = 1;

    float* voldata = 0;
    do {
        fin.getline(line,sizeof(line)-1);
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            printf("%s\n", line);
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
                isrect = 1;
            }
            else if (sscanf(start, "object %d class array type float rank 1 shape 3 items %d data follows", &dummy, &nxy) == 2) {
                isrect = 0;
                double xx, yy, zz;
                for (int i=0; i < nxy; i++) {
                    fin.getline(line, sizeof(line));
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &xx, &yy, &zz) == 3) {
                        xymesh.addNode( Rappture::Node2D(xx,yy) );
                    }
                }
                char mesg[256];
                sprintf(mesg,"test");
                result.error(mesg);
                return result;

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
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                }
                else if (!isrect && (npts != nxy*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, npts);
                    return result.error(mesg);
                }
                break;
            }
            else if (sscanf(start, "object %d class array type %s rank 0 times %d data follows", &dummy, type, &npts) == 3) {
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

                    if (dval[p] < vmin) vmin = dval[p];
                    if (dval[p] > vmax) vmax = dval[p];
                    if (dval[p] != 0.0f && dval[p] < nzero_min)
                    {
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
            printf("test2\n");
                        fflush(stdout);
            if (dv == 0.0) { dv = 1.0; }
            for (int i = 0; i < count; ++i)
            {
                v = data[ngen];
                // scale all values [0-1], -1 => out of bounds
                v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                data[ngen] = v;
                ngen += 4;
            }

            // Compute the gradient of this data.  BE CAREFUL: center
            // calculation on each node to avoid skew in either direction.
            ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int ix=0; ix < nx; ix++) {
                        // gradient in x-direction
                        double valm1 = (ix == 0) ? 0.0 : data[ngen - 4];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen + 4];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = valp1-valm1; // assume dx=1
                            //data[ngen+1] = ((valp1-valm1) + 1) *  0.5; // assume dx=1
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            data[ngen+2] = valp1-valm1; // assume dx=1
                            //data[ngen+2] = ((valp1-valm1) + 1) *  0.5; // assume dy=1
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            data[ngen+3] = valp1-valm1; // assume dx=1
                            //data[ngen+3] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1
                        }

                        ngen += 4;
                    }
                }
            }

            dx = nx;
            dy = ny;
            dz = nz;
	    NanoVis::load_volume(index, nx, ny, nz, 4, data,
                vmin, vmax, nzero_min);

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
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-4];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+4];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            //data[ngen+1] = valp1-valm1; // assume dx=1
                            data[ngen+1] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            //data[ngen+2] = valp1-valm1; // assume dy=1
                            data[ngen+2] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            //data[ngen+3] = valp1-valm1; // assume dz=1
                            data[ngen+3] = ((valp1-valm1) + 1.0) * 0.5; // assume dz=1
                        }

                        ngen += 4;
                    }
                }
            }

	    NanoVis::load_volume(index, nx, ny, nz, 4, data,
                field.valueMin(), field.valueMax(), nzero_min);

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
    Rappture::Outcome result;

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;

    int isrect = 1;

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

            float *cdata = new float[nx*ny*nz];
            int ngen = 0;
            double nzero_min = 0.0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) 
                    {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        if (v != 0.0f && v < nzero_min)
                        {
                            nzero_min = v;
                        }

                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : v;

                        cdata[ngen] = v;
                        ++ngen;
                    }
                }
            }

            float* data = computeGradient(cdata, nx, ny, nz, field.valueMin(), field.valueMax());

            // Compute the gradient of this data.  BE CAREFUL: center
            /*
            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            double nzero_min = 0.0;
            for (int iz=0; iz < nz; iz++) {
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
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-4];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+4];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = ((valp1-valm1) + 1) *  0.5; // assume dx=1
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            //data[ngen+2] = valp1-valm1; // assume dy=1
                            data[ngen+2] = ((valp1-valm1) + 1) *  0.5; // assume dx=1
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            //data[ngen+3] = valp1-valm1; // assume dz=1
                            data[ngen+3] = ((valp1-valm1) + 1) *  0.5; // assume dz=1
                        }

                        ngen += 4;
                    }
                }
            }
            */

	    NanoVis::load_volume(index, nx, ny, nz, 4, data,
                field.valueMin(), field.valueMax(), nzero_min);

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
                            //data[ngen+1] = valp1-valm1; // assume dx=1
                            data[ngen+1] = ((valp1-valm1) + 1) *  0.5; // assume dx=1
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            //data[ngen+2] = valp1-valm1; // assume dy=1
                            data[ngen+2] = ((valp1-valm1) + 1) *  0.5; // assume dy=1
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            //data[ngen+3] = valp1-valm1; // assume dz=1
                            data[ngen+3] = ((valp1-valm1) + 1) *  0.5; // assume dz=1
                        }

                        ngen += 4;
                    }
                }
            }

	        NanoVis::load_volume(index, nx, ny, nz, 4, data,
                field.valueMin(), field.valueMax(), nzero_min);

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
