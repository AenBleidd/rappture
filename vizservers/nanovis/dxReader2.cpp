
#include "dxReaderCommon.h"

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


// FIXME: move newCoolInterplation to the Rappture::DX object
// this definition is kept only so we can test newCoolInterpolation
typedef struct {
    int nx;
    int ny;
    int nz;
    double dataMin;
    double dataMax;
    double nzero_min;
    float* data;
} dataInfo;

// This is legacy code that can be removed during code cleanup
int getInterpPos(  float* numPts, float *origin,
                    float* max, int rank, float* arr)
{
    float dn[rank];
    int i = 0;
    float x = 0;
    float y = 0;
    float z = 0;

    // dn holds the delta you want for interpolation
    for (i = 0; i < rank; i++) {
        // dn[i] = (max[i] - origin[i]) / (numPts[i] - 1);
        dn[i] = (max[i] - origin[i]) / (numPts[i]);
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

    return 0;
}

// This is legacy code that can be removed during code cleanup
int getInterpData(  int rank, float* numPts, float* max, float* dxpos, Object* dxobj,
                    double* dataMin, double* dataMax, float** result)
{
    int pts = int(numPts[0]*numPts[1]*numPts[2]);
    int interppts = pts;
    int arrSize = interppts*rank;
    float interppos[arrSize];
    Interpolator interpolator;

//     commented this out to see what happens when we use the dxpos array
//     which holds the positions dx generated for interpolation.
//
    // generate the positions we want to interpolate on
//    getInterpPos(numPts,dxpos,max,rank,interppos);
//    fprintf(stdout, "after getInterpPos\n");
//    fflush(stdout);

    // build the interpolator and interpolate
    interpolator = DXNewInterpolator(*dxobj,INTERP_INIT_IMMEDIATE,-1.0);
    fprintf(stdout, "after DXNewInterpolator=%x\n", (unsigned int)interpolator);
    fflush(stdout);
//    DXInterpolate(interpolator,&interppts,interppos,*result);
    DXInterpolate(interpolator,&interppts,dxpos,*result);
    fprintf(stdout,"interppts = %i\n",interppts);
    fflush(stdout);

    // print debug info
    for (int lcv = 0, pt = 0; lcv < pts; lcv+=3,pt+=9) {
        fprintf(stdout,
            "(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n",
//            interppos[pt],interppos[pt+1],interppos[pt+2], (*result)[lcv],
//            interppos[pt+3],interppos[pt+4],interppos[pt+5],(*result)[lcv+1],
//            interppos[pt+6],interppos[pt+7],interppos[pt+8],(*result)[lcv+2]);
            dxpos[pt],dxpos[pt+1],dxpos[pt+2], (*result)[lcv],
            dxpos[pt+3],dxpos[pt+4],dxpos[pt+5],(*result)[lcv+1],
            dxpos[pt+6],dxpos[pt+7],dxpos[pt+8],(*result)[lcv+2]);
        fflush(stdout);
    }

    for (int lcv = 0; lcv < pts; lcv=lcv+3) {
        if ((*result)[lcv] < *dataMin) {
            *dataMin = (*result)[lcv];
        }
        if ((*result)[lcv] > *dataMax) {
            *dataMax = (*result)[lcv];
        }
    }

    return 0;
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
int idx2xyz(const int *axisLen, int idx, int *xyz) {
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
int xyz2idx(const int *axisLen, int *xyz, int *idx) {
    int mult = 1;
    int axis = 0;

    *idx = 0;

    for (axis = 0; axis < 3; axis++) {
        *idx = *idx + xyz[axis]*mult;
        mult = mult*axisLen[axis];
    }

    return 0;
}

Rappture::FieldRect3D* fillRectMesh(const int* numPts, const float* delta, const float* dxpos, const float* data)
{
    Rappture::Mesh1D xgrid(dxpos[0], dxpos[0]+numPts[0]*delta[0], (int)(ceil(numPts[0])));
    Rappture::Mesh1D ygrid(dxpos[1], dxpos[1]+numPts[1]*delta[1], (int)(ceil(numPts[1])));
    Rappture::Mesh1D zgrid(dxpos[2], dxpos[2]+numPts[2]*delta[2], (int)(ceil(numPts[2])));
    Rappture::FieldRect3D* field = new Rappture::FieldRect3D(xgrid, ygrid, zgrid);

    int i = 0;
    int idx = 0;
    int xyz[] = {0,0,0};

    //FIXME: This can be switched to 3 for loops to avoid the if checks
    for (i=0; i<numPts[0]*numPts[1]*numPts[2]; i++,xyz[2]++) {
        if (xyz[2] >= numPts[2]) {
            xyz[2] = 0;
            xyz[1]++;
            if (xyz[1] >= numPts[1]) {
                xyz[1] = 0;
                xyz[0]++;
                if (xyz[0] >= numPts[0]) {
                    xyz[0] = 0;
                }
            }
        }
        xyz2idx(numPts,xyz,&idx);
        field->define(idx, data[i]);
        fprintf(stdout,"xyz = {%i,%i,%i}\tidx = %i\tdata[%i] = % 8e\n",xyz[0],xyz[1],xyz[2],idx,i,data[i]);
        fflush(stdout);
    }

    return field;
}

float* oldCrustyInterpolation(int* nx, int* ny, int* nz, double* dx, double* dy, double* dz, const float* dxpos, Rappture::FieldRect3D* field, double* nzero_min)
{
    // figure out a good mesh spacing
    int nsample = 30;
    *dx = field->rangeMax(Rappture::xaxis) - field->rangeMin(Rappture::xaxis);
    *dy = field->rangeMax(Rappture::yaxis) - field->rangeMin(Rappture::yaxis);
    *dz = field->rangeMax(Rappture::zaxis) - field->rangeMin(Rappture::zaxis);
    double dmin = pow((*dx**dy**dz)/(nsample*nsample*nsample), 0.333);

    *nx = (int)ceil(*dx/dmin);
    *ny = (int)ceil(*dy/dmin);
    *nz = (int)ceil(*dz/dmin);

#ifndef NV40
    // must be an even power of 2 for older cards
    *nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
    *ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
    *nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

    float *cdata = new float[*nx**ny**nz];
    int ngen = 0;
    *nzero_min = 0.0;
    for (int iz=0; iz < *nz; iz++) {
        double zval = dxpos[2] + iz*dmin;
        for (int iy=0; iy < *ny; iy++) {
            double yval = dxpos[1] + iy*dmin;
            for (int ix=0; ix < *nx; ix++)
            {
                double xval = dxpos[0] + ix*dmin;
                double v = field->value(xval,yval,zval);

                if (v != 0.0f && v < *nzero_min)
                {
                    *nzero_min = v;
                }

                // scale all values [0-1], -1 => out of bounds
                v = (isnan(v)) ? -1.0 : v;

                cdata[ngen] = v;
                ++ngen;
            }
        }
    }

    float* data = computeGradient(cdata, *nx, *ny, *nz, field->valueMin(), field->valueMax());

    delete[] cdata;
    return data;
}

// FIXME: move newCoolInterplation to the Rappture::DX object
int
newCoolInterpolation(int pts, dataInfo* inData, dataInfo* outData, float* max, float* dxpos, float* delta, float* numPts, double* dx, double* dy, double* dz)
{
    // wouldnt dataMin have the non-zero min of the data?
    //double nzero_min = 0.0;
    outData->nzero_min = inData->dataMin;

    // need to rewrite this starting here!!! dsk
    // figure out a good mesh spacing
    // dxpos[0,1,2] are the origins for x,y,z axis
    *dx = max[0] - dxpos[0];
    *dy = max[1] - dxpos[1];
    *dz = max[2] - dxpos[2];
    double dmin = pow((*dx**dy**dz)/(pts), (double)(1.0/3.0));

    fprintf(stdout, "dx = %f    dy = %f    dz = %f\n",*dx,*dy,*dz);
    fprintf(stdout, "dmin = %f\n",dmin);
    fprintf(stdout, "max = [%f,%f,%f]\n",max[0],max[1],max[2]);
    fprintf(stdout, "delta = [%f,%f,%f]\n",delta[0],delta[1],delta[2]);
    fprintf(stdout, "numPts = [%f,%f,%f]\n",numPts[0],numPts[1],numPts[2]);
    fflush(stdout);


    outData->nx = (int)ceil(numPts[0]);
    outData->ny = (int)ceil(numPts[1]);
    outData->nz = (int)ceil(numPts[2]);

    fprintf(stdout, "nx = %i    ny = %i    nz = %i\n",outData->nx,outData->ny,outData->nz);

    outData->data = new float[outData->nx*outData->ny*outData->nz];
    for (int i=0; i<(outData->nx*outData->ny*outData->nz); i+=1) {
        // scale all values [0,1], -1 => out of bounds
        outData->data[i] = (isnan(inData->data[i])) ? -1.0 :
            (inData->data[i] - inData->dataMin)/(inData->dataMax-inData->dataMin);
        fprintf(stdout, "outData[%i] = % 8e\tinData[%i] = % 8e\n",i,outData->data[i],i,inData->data[i]);
    }

    return 0;
}

/* Load a 3D volume from a dx-format file the new way
 */
Rappture::Outcome
load_volume_stream_odx(int index, const char *buf, int nBytes) {
    Rappture::Outcome outcome;

    char dxfilename[128];

    if (nBytes == 0) {
        return outcome.error("data not found in stream");
    }

    // write the dx file to disk, because DXImportDX takes a file name
    // george suggested using a pipe here
    sprintf(dxfilename, "/tmp/dx%d.dx", getpid());

    FILE *f;

    f = fopen(dxfilename, "w");
    fwrite(buf, sizeof(char), nBytes, f);
    fclose(f);

    Rappture::DX dxObj(dxfilename);

    if (remove(dxfilename) != 0) {
        fprintf(stderr, "Error deleting dx file: %s\n", dxfilename);
        fflush(stderr);
    }

    // store our data in a rappture structure to be interpolated
    Rappture::FieldRect3D* field = fillRectMesh(dxObj.axisLen(),
                                                dxObj.delta(),
                                                dxObj.positions(),
                                                dxObj.data());

    int nx = 0;
    int ny = 0;
    int nz = 0;
    double dx = 0.0;
    double dy = 0.0;
    double dz = 0.0;
    double nzero_min = 0.0;
    float* data = oldCrustyInterpolation(&nx,&ny,&nz,&dx,&dy,&dz,
                                        dxObj.positions(),field,&nzero_min);

    double dataMin = field->valueMin();
    double dataMax = field->valueMax();


    for (int i=0; i<nx*ny*nz; i++) {
        fprintf(stdout,"enddata[%i] = %lg\n",i,data[i]);
        fflush(stdout);
    }

    fprintf(stdout,"End Data Stats index = %i\n",index);
    fprintf(stdout,"nx = %i ny = %i nz = %i\n",nx,ny,nz);
    fprintf(stdout,"dx = %lg dy = %lg dz = %lg\n",dx,dy,dz);
    fprintf(stdout,"dataMin = %lg\tdataMax = %lg\tnzero_min = %lg\n", dataMin,dataMax,nzero_min);
    fflush(stdout);
    NanoVis::load_volume(index, nx, ny, nz, 4, data, dataMin, dataMax,
                             nzero_min);

    // free the result array
    // delete[] data;
    // delete[] result;
    // delete field;

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    NanoVis::volume[index]->move(Vector3(dx0, dy0, dz0));

    return outcome;
}
