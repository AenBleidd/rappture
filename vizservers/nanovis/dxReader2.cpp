
//opendx (libdx4-dev) headers
#include <dx/dx.h>

#include <stdio.h>
#include <iostream>
#include <fstream>

// nanovis headers
#include "Nv.h"
#include "nanovis.h"

// rappture headers
#include "RpEncode.h"
#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"

typedef struct {
    int n;
    int rank;
    int shape[3];
    Category category;
    Type type;
} arrayInfo;

int initArrayInfo(arrayInfo* a)
{
    a->n = 0;
    a->rank = 0;
    a->shape[0] = 0;
    a->shape[1] = 0;
    a->shape[2] = 0;
    return 0;
}

int getInterpPos2(  float* numPts, float *origin,
                    float* max, int rank, float* arr)
{
    float dn[rank];
    int i = 0;
    float x = 0;
    float y = 0;
    float z = 0;

    // dn holds the delta you want for interpolation
    for (i = 0; i < rank; i++) {
        dn[i] = (max[i] - origin[i]) / (numPts[i] - 1);
    }

    // points are calculated based on the new delta
    i = 0;
    for (x = origin[0]; x < numPts[0]; x++) {
        for (y = origin[1]; y < numPts[1]; y++) {
            for (z = origin[2]; z < numPts[2]; z++) {
                arr[i++] = x*dn[0];
                arr[i++] = y*dn[1];
                arr[i++] = z*dn[2];
                fprintf(stderr, "(%f,%f,%f)\n",arr[i-3],arr[i-2],arr[i-1]);
            }
        }
    }

    /*
    for (z = origin[2]-(numPts[2]/2.0); z < origin[2]+(numPts[2]/2.0); z++) {
        for (y = origin[1]-(numPts[1]/2.0); y < origin[1]+(numPts[1]/2.0); y++) {
            for (x = origin[0]-(numPts[0]/2.0); x < origin[0]+(numPts[0]/2.0); x++) {
                arr[i++] = x*dn[0];
                arr[i++] = y*dn[1];
                arr[i++] = z*dn[2];
                fprintf(stderr, "(%f,%f,%f)\n",arr[i-3],arr[i-2],arr[i-1]);
            }
        }
    }
    */

    return 0;
}


int getDataStats(   int n, int rank, int shape,
                    float* data, float* delta, float* max)
{
    int lcv = 0;
    int r = rank*shape;

    // initialize the max array and delta array
    // max holds the maximum value found for each index
    // delta holds the difference between each entry's value
    for (lcv = 0; lcv < r; lcv++) {
        max[lcv] = data[lcv];
        delta[lcv] = data[lcv];
    }

    for (lcv=lcv; lcv < n*r; lcv++) {
        if (data[lcv] > max[lcv%r]) {
            max[lcv%r] = data[lcv];
        }
        if (delta[lcv%r] == data[lcv%r]) {
            if (data[lcv] != data[lcv-r]) {
                delta[lcv%r] = data[lcv] - data[lcv-r];
            }
        }
    }

    return lcv;
}

/*
 * Generalized linear to cartesian transformation function
 * This function should work with n dimensional matricies
 * with varying axis lengths.
 *
 * Inputs:
 * axisLen - array containing the length of each axis
 * idx - the index in a linear array from which x,y,z values are derived
 * Outputs:
 * xyz - array containing the values of the x, y, and z axis
 */
int idx2xyz(int *axisLen, int idx, int *xyz) {
    int mult = 1;
    int axis = 0;

    for (axis = 0; axis < 3; axis++) {
        xyz[axis] = (idx/mult) % axisLen[axis];
        mult = mult*axisLen[axis];
    }

    return 0;
}

/*
 * Generalized cartesian to linear transformation function
 * This function should work with n dimensional matricies
 * with varying axis lengths.
 *
 * Inputs:
 * axisLen - array containing the length of each axis
 * xyz - array containing the values of the x, y, and z axis
 * Outputs:
 * idx - the index that represents xyz in a linear array
 */
int xyz2idx(int *axisLen, int *xyz, int *idx) {
    int mult = 1;
    int axis = 0;

    *idx = 0;

    for (axis = 0; axis < 3; axis++) {
        *idx = *idx + xyz[axis]*mult;
        mult = mult*axisLen[axis];
    }

    return 0;
}

/* Load a 3D volume from a dx-format file the new way
 */
Rappture::Outcome
load_volume_stream_odx(int index, const char *buf, int nBytes) {
    Rappture::Outcome outcome;

    int nx, ny, nz;
    double dx, dy, dz;
    char dxfilename[128];

    Object dxobj;
    Array dxarr;
    arrayInfo dataInfo;
    arrayInfo posInfo;

    if (nBytes == 0) {
        return outcome.error("data not found in stream");
    }

    // write the dx file to disk, because DXImportDX takes a file name
    // george suggested using a pipe here
    sprintf(dxfilename, "/tmp/dx%d.dx", getpid());

#ifdef notdef
    std::ofstream ftmp(dxfilename);
    fin.seekg(0,std::ios::end);
    int finLen = fin.tellg();
    fin.seekg(0,std::ios::beg);
    char *finBuf = new char[finLen+1];
    finBuf[finLen] = '\0';
    fprintf(stderr, "length = %d\n",finLen);
    fin.read(finBuf,finLen);
    ftmp << finBuf;
    ftmp.close();
#else
    FILE *f;
    
    f = fopen(dxfilename, "w");
    fwrite(buf, sizeof(char), nBytes, f);
    fclose(f);
#endif

    fprintf(stdout, "Calling DXImportDX(%s)\n", dxfilename);
    fflush(stdout);
    dxobj = DXImportDX(dxfilename,NULL,NULL,NULL,NULL);
    fprintf(stdout, "dxobj=%x\n", dxobj);
    fflush(stdout);
    initArrayInfo(&dataInfo);
    dxarr = (Array)DXGetComponentValue((Field) dxobj, (char *)"data");

    DXGetArrayInfo( dxarr, &dataInfo.n, &dataInfo.type,
                    &dataInfo.category, &dataInfo.rank, dataInfo.shape);
    fprintf(stdout, "after DXGetArrayInfo\n");
    fflush(stdout);

    initArrayInfo(&posInfo);
    dxarr = (Array) DXGetComponentValue((Field) dxobj, (char *)"positions");
    DXGetArrayInfo( dxarr, &posInfo.n, &posInfo.type, 
                    &posInfo.category, &posInfo.rank, posInfo.shape);
    fprintf(stdout, "after DXGetArrayInfo\n");
    fflush(stdout);
    float* dxpos = (float*) DXGetArrayData(dxarr);

    float delta[] = {0.0,0.0,0.0};
    float max[] = {0.0,0.0,0.0};
    getDataStats(posInfo.n,posInfo.rank,posInfo.shape[0],dxpos,delta,max);
    fprintf(stdout, "after getDataStats\n");

    int rank = 3;
    int nsample = 30;
    float numPts[] = { nsample, nsample, nsample};
    int pts = int(numPts[0]*numPts[1]*numPts[2]);
    int interppts = pts;
    int arrSize = interppts*rank;
    float interppos[arrSize];
    float result[interppts];
    Interpolator interpolator;
    double dataMin = 0.0;
    double dataMax = 0.0;
    double nzero_min = 0.0;
    
    getInterpPos2(numPts,dxpos,max,rank,interppos);
    fprintf(stdout, "after getInterpPos2\n");
    fflush(stdout);
    interpolator = DXNewInterpolator(dxobj,INTERP_INIT_IMMEDIATE,-1.0);
    fprintf(stdout, "after DXNewInterpolator=%x\n", interpolator);
    fflush(stdout);
    DXInterpolate(interpolator,&interppts,interppos,result);
    fprintf(stdout,"interppts = %i\n",interppts);
    fflush(stdout);
    for (int lcv = 0, pt = 0; lcv < pts; lcv+=3,pt+=9) {
	fprintf(stdout,
		"(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n",
		interppos[pt],interppos[pt+1],interppos[pt+2], result[lcv],
		interppos[pt+3],interppos[pt+4],interppos[pt+5],result[lcv+1],
		interppos[pt+6],interppos[pt+7],interppos[pt+8],result[lcv+2]);
    fflush(stdout);
    }
    
    for (int lcv = 0; lcv < pts; lcv=lcv+3) {
	if (result[lcv] < dataMin) {
	    dataMin = result[lcv];
	}
	if (result[lcv] > dataMax) {
	    dataMax = result[lcv];
	}
    }
    
    // i dont understand what nzero_min is for
    // i assume it means non-zero min, but its the non-zero min of what?
    // wouldnt dataMin have the non-zero min of the data?
    nzero_min = dataMin;
    
    // need to rewrite this starting here!!! dsk
    // figure out a good mesh spacing
    // dxpos[0,1,2] are the origins for x,y,z axis
    dx = max[0] - dxpos[0];
    dy = max[1] - dxpos[1];
    dz = max[2] - dxpos[2];
    double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);
    
    fprintf(stdout, "dx = %f    dy = %f    dz = %f\n",dx,dy,dz);
    fprintf(stdout, "dmin = %f\n",dmin);
    fflush(stdout);
    nx = (int)ceil(dx/dmin);
    ny = (int)ceil(dy/dmin);
    nz = (int)ceil(dz/dmin);
    
    fprintf(stdout, "nx = %i    ny = %i    nz = %i\n",nx,ny,nz);
#ifndef NV40
    // must be an even power of 2 for older cards
    nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
    ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
    nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif
    
    fprintf(stdout, "nx = %i    ny = %i    nz = %i\n",nx,ny,nz);
    float *data = new float[nx*ny*nz];
    for (int i=0; i<(nx*ny*nz); i+=1) {
	// scale all values [0,1], -1 => out of bounds
	data[i] = (isnan(result[i])) ? -1.0 :
	    (result[i] - dataMin)/(dataMax-dataMin);
	fprintf(stdout, "data[%i] = % 8e\n",i,data[i]);
    }
    NanoVis::load_volume(index, nx, ny, nz, 1, data, dataMin, dataMax, 
			 nzero_min);

    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    NanoVis::volume[index]->move(Vector3(dx0, dy0, dz0));
    return outcome;
}

