
#include <stdio.h>
#include <string.h>
#include <rappture.h>
#include <tcl.h>
#include "Unirect.h"
#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"
#include "Nv.h"
#include "nanovis.h"
#include "FlowCmd.h"

    

bool
ScaleVectorFieldData(Rappture::Outcome &context, Rappture::Unirect3d *dataPtr)
{
    float *values = dataPtr->values();
    float *nValues = dataPtr->nValues();
#ifdef notdef
    double max_x = -1e21, min_x = 1e21;
    double max_y = -1e21, min_y = 1e21;
    double max_z = -1e21, min_z = 1e21;
#endif
    double max_mag = -1e21, min_mag = 1e21;
    for (size_t i = 0; i < nValues; i += 3) {
	double vx, vy, vz, vm;

	vx = values[i];
	vy = values[i+1];
	vz = values[i+2];
		    
	vm = sqrt(vx*vx + vy*vy + vz*vz);
	if (vm > max_mag) {
	    max_mag = vm;
	}
	if (vm < min_mag) {
	    min_mag = vm;
	}
    }
    
    size_t n = 4* xNum * yNum * zNum;
    float *data = new float[n];
    memset(data, 0, sizeof(float) * n);
    
    fprintf(stderr, "generating %dx%dx%d = %d points\n", 
	    xNum, yNum, zNum, xNum * yNum * zNum);
    
    float *destPtr = data;
    float *srcPtr = values;
    for (size_t i = 0; i < zNum; i++) {
	for (size_t j = 0; j < yNum; j++) {
	    for (size_t k = 0; k < xNum; k++) {
		double vx, vy, vz, vm;

		vx = srcPtr[0];
		vy = srcPtr[1];
		vz = srcPtr[2];
		
		vm = sqrt(vx*vx + vy*vy + vz*vz);
		
		destPtr[0] = vm / max_mag; 
		destPtr[1] = vx /(2.0*max_mag) + 0.5; 
		destPtr[2] = vy /(2.0*max_mag) + 0.5; 
		destPtr[3] = vz /(2.0*max_mag) + 0.5; 
		srcPtr += 3;
		destPtr += 4;
	    }
	}
    }
    dataPtr->SetValues(xMin, xMax, xNum, yMin, yMax(yMax);
    dataPtr->yNum(yNum);
    dataPtr->yMin(zMin);
    dataPtr->yMax(zMax);
    dataPtr->yNum(zNum);
    dataPtr->values(data);
    dataPtr->minMag = min_mag;
    dataPtr->maxMag = max_mag;
    return true;
}

bool
ScaleResampledVectorFieldData(Rappture::Outcome &context, 
			      Rappture::Unirect3d *dataPtr)
{
    Rappture::Mesh1D xgrid(dataPtr->xMin(), dataPtr->xMax(), dataPtr->xNum());
    Rappture::Mesh1D ygrid(dataPtr->yMin(), dataPtr->yMax(), dataPtr->yNum());
    Rappture::Mesh1D zgrid(dataPtr->zMin(), dataPtr->zMax(), dataPtr->zNum());
    Rappture::FieldRect3D xfield(xgrid, ygrid, zgrid);
    Rappture::FieldRect3D yfield(xgrid, ygrid, zgrid);
    Rappture::FieldRect3D zfield(xgrid, ygrid, zgrid);

#ifdef notdef
    double max_x = -1e21, min_x = 1e21;
    double max_y = -1e21, min_y = 1e21;
    double max_z = -1e21, min_z = 1e21;
#endif

    double max_mag, min_mag, nzero_min;
    max_mag = -1e21, nzero_min = min_mag = 1e21;
    size_t i, j;
    for (i = j = 0; i < dataPtr->nValues(); i += 3, j++) {
	double vx, vy, vz, vm;

	vx = dataPtr->values[i];
	vy = dataPtr->values[i+1];
	vz = dataPtr->values[i+2];
	
	xfield.define(j, vx);
	yfield.define(j, vy);
	zfield.define(j, vz);

	vm = sqrt(vx*vx + vy*vy + vz*vz);
	if (vm > max_mag) {
	    max_mag = vm;
	}
	if (vm < min_mag) {
	    min_mag = vm;
	}
	if ((vm != 0.0f) && (vm < nzero_min)) {
	    nzero_min = vm;
	}
    }
    
    // figure out a good mesh spacing
    int nSamples = 30;
    double dx, dy, dz;
    dx = xfield.rangeMax(Rappture::xaxis) - xfield.rangeMin(Rappture::xaxis);
    dy = xfield.rangeMax(Rappture::yaxis) - xfield.rangeMin(Rappture::yaxis);
    dz = xfield.rangeMax(Rappture::zaxis) - xfield.rangeMin(Rappture::zaxis);

    double dmin;
    dmin = pow((dx*dy*dz)/(nSamples*nSamples*nSamples), 0.333);
    
    printf("dx:%lf dy:%lf dz:%lf dmin:%lf\n", dx, dy, dz, dmin);
    
    /* Recompute new number of points for each axis. */
    xNum = (int)ceil(dx/dmin);
    yNum = (int)ceil(dy/dmin);
    zNum = (int)ceil(dz/dmin);
    
#ifndef NV40
    // must be an even power of 2 for older cards
    xNum = (int)pow(2.0, ceil(log10((double)xNum)/log10(2.0)));
    yNum = (int)pow(2.0, ceil(log10((double)yNum)/log10(2.0)));
    zNum = (int)pow(2.0, ceil(log10((double)zNum)/log10(2.0)));
#endif

    size_t n = 4 * xNum * yNum * zNum;
    float *data = new float[n];
    memset(data, 0, sizeof(float) * n);
    
    fprintf(stderr, "generating %dx%dx%d = %d points\n", 
	    xNum, yNum, zNum, xNum * yNum * zNum);
    
    // Generate the uniformly sampled rectangle that we need for a volume
    float *destPtr = data;
    for (size_t i = 0; i < zNum; i++) {
	double z;

	z = zMin + (i * dmin);
	for (size_t j = 0; j < yNum; j++) {
	    double y;
		
	    y = yMin + (j * dmin);
	    for (size_t k = 0; k < xNum; k++) {
		double x;
		double vx, vy, vz, vm;

		x = xMin + (k * dmin);
		vx = xfield.value(x, y, z);
		vy = yfield.value(x, y, z);
		vz = zfield.value(x, y, z);
		vm = sqrt(vx*vx + vy*vy + vz*vz);
		
		destPtr[0] = vm / max_mag; 
		destPtr[1] = vx /(2.0*max_mag) + 0.5; 
		destPtr[2] = vy /(2.0*max_mag) + 0.5; 
		destPtr[3] = vz /(2.0*max_mag) + 0.5; 
		destPtr += 4;
	    }
	}
    }
    dataPtr->zMin()
    flowPtr->xMin = xMin;
    flowPtr->xMax = xMax;
    flowPtr->xNum = xNum;
    flowPtr->yMin = yMin;
    flowPtr->yMax = yMax;
    flowPtr->yNum = yNum;
    flowPtr->yMin = zMin;
    flowPtr->yMax = zMax;
    flowPtr->yNum = zNum;
    flowPtr->values = data;
    flowPtr->minMag = min_mag;
    flowPtr->maxMag = max_mag;
    return true;
}

bool
SetVectorFieldDataFromUnirect3d(Rappture::Outcome &context, 
				Rappture::Unirect3d &grid, FlowCmd *flowPtr)
{
    flowPtr->dataPtr = gridPtr;
    return SetVectorFieldData(context, 
			      grid.xMin(), grid.xMax(), grid.xNum(), 
			      grid.yMin(), grid.yMax(), grid.yNum(), 
			      grid.zMin(), grid.zMax(), grid.zNum(), 
			      grid.nValues(), grid.values(), flowPtr);
 }


/*
 * Load a 3D vector field from a dx-format file
 */
bool
load_vector_stream2(Rappture::Outcome &context, size_t length, char *string, 
		    Unirect3d *dataPtr)
{
    Unirect3d data;
    int nx, ny, nz, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char *p, *endPtr;

    dx = dy = dz = 0.0;         // Suppress compiler warning.
    x0 = y0 = z0 = 0.0;		// May not have an origin line.
    for (p = string, endPtr = p + length; p < endPtr; /*empty*/) {
	char *line;

	line = getline(&p, endPtr);
        if (line == endPtr) {
	    break;
	}
        if (*line == '#') {  // skip comment lines
	    continue;
	}
	if (sscanf(line, "object %*d class gridpositions counts %d %d %d", 
		   &nx, &ny, &nz) == 4) {
	    printf("w:%d h:%d d:%d\n", nx, ny, nz);
	    // found grid size
	} else if (sscanf(line, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
	    // found origin
	} else if (sscanf(line, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
	    // found one of the delta lines
	    if (ddx != 0.0) {
		dx = ddx;
	    } else if (ddy != 0.0) {
		dy = ddy;
	    } else if (ddz != 0.0) {
		dz = ddz;
	    }
	} else if (sscanf(line, "object %*d class array type %*s shape 3"
		" rank 1 items %d data follows", &npts) == 3) {
	    printf("point %d\n", npts);
	    if (npts != nx*ny*nz) {
		context.addError("inconsistent data: expected %d points"
				" but found %d points", nx*ny*nz, npts);
		return false;
	    }
	    break;
	} else if (sscanf(line, "object %*d class array type %*s rank 0"
		" times %d data follows", &npts) == 3) {
	    if (npts != nx*ny*nz) {
		context.addError("inconsistent data: expected %d points"
				" but found %d points", nx*ny*nz, npts);
		return false;
	    }
	    break;
	}
    }

    // read data points
    float* srcData = new float[nx * ny * nz * 3];
    if (p >= endPtr) {
        std::cerr << "WARNING: data not found in stream" << std::endl;
	return true;
    }
#ifdef notdef
    double max_x = -1e21, min_x = 1e21;
    double max_y = -1e21, min_y = 1e21;
    double max_z = -1e21, min_z = 1e21;
#endif
    double max_mag = -1e21, min_mag = 1e21;
    int nRead = 0;
    for (int ix=0; ix < nx; ix++) {
	for (int iy=0; iy < ny; iy++) {
	    for (int iz=0; iz < nz; iz++) {
		char *line;
		double vx, vy, vz, vm;

		if ((p == endPtr) || nRead > npts) {
		    break;
		}
		line = getline(&p, endPtr);
		if (sscanf(line, "%lg %lg %lg", &vx, &vy, &vz) == 3) {
		    int nindex = (iz*nx*ny + iy*nx + ix) * 3;
		    srcData[nindex] = vx;
		    //if (srcData[nindex] > max_x) max_x = srcData[nindex];
		    //if (srcData[nindex] < min_x) min_x = srcData[nindex];
		    ++nindex;
		    
		    srcData[nindex] = vy;
		    //if (srcData[nindex] > max_y) max_y = srcData[nindex];
		    //if (srcData[nindex] < min_y) min_y = srcData[nindex];
		    ++nindex;
		    
		    srcData[nindex] = vz;
		    //if (srcData[nindex] > max_z) max_z = srcData[nindex];
		    //if (srcData[nindex] < min_z) min_z = srcData[nindex];
		    
		    vm = sqrt(vx*vx + vy*vy + vz*vz);
		    if (vm > max_mag) {
			max_mag = vm;
		    }
		    if (vm < min_mag) {
			min_mag = vm;
		    }
		    ++nRead;
		}
	    }
	}
    }
    
    // make sure that we read all of the expected points
    if (nRead != nx*ny*nz) {
	context.addError("inconsistent data: expected %d points"
			" but found %d points", nx*ny*nz, npts);
	return false;
    }
    bool status;
    status = SetVectorFieldData(context, x0, x0 + nx, nx, y0, y0 + ny, 
	ny, z0, z0 + ny, ny, nRead, srcData, flowPtr);
    delete [] srcData;
    return status;
}

