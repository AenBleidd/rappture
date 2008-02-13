
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

// keep this header around for testing
/*
 * Load a 3D vector field from a dx-format file
 */
void
load_vector_stream(int index, std::istream& fin)
{}


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
load_volume_stream(int index, std::iostream& fin) {
    Rappture::Outcome result;

    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char dxfilename[128];

    Object dxobj;
    Array dxarr;
    arrayInfo dataInfo;
    arrayInfo posInfo;

    if (fin.eof()) {
        return result.error("data not found in stream");
    }

    // write the dx file to disk, because DXImportDX takes a file name
    // george suggested using a pipe here
    sprintf(dxfilename, "/tmp/dx%d.dx", getpid());

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

    dxobj = DXImportDX(dxfilename,NULL,NULL,NULL,NULL);

    initArrayInfo(&dataInfo);
    dxarr = (Array) DXGetComponentValue((Field) dxobj, "data");

    DXGetArrayInfo( dxarr, &dataInfo.n, &dataInfo.type, \
                    &dataInfo.category, &dataInfo.rank, dataInfo.shape);

    float* dxdata = (float*) DXGetArrayData(dxarr);

    initArrayInfo(&posInfo);
    dxarr = (Array) DXGetComponentValue((Field) dxobj, "positions");
    DXGetArrayInfo( dxarr, &posInfo.n, &posInfo.type, \
                    &posInfo.category, &posInfo.rank, posInfo.shape);
    float* dxpos = (float*) DXGetArrayData(dxarr);

    float delta[] = {0.0,0.0,0.0};
    float max[] = {0.0,0.0,0.0};
    getDataStats(posInfo.n,posInfo.rank,posInfo.shape[0],dxpos,delta,max);

    if (0) {
        // x0 is the origin's x value
        // x0+nx*dx is the largest x value
        // nx is the number of points

        x0 = dxpos[0];
        y0 = dxpos[1];
        z0 = dxpos[2];
        nx = abs(int(max[0]/delta[0]))+1;
        ny = abs(int(max[1]/delta[1]))+1;
        nz = abs(int(max[2]/delta[2]))+1;
        dx = delta[0];
        dy = delta[1];
        dz = delta[2];

        fprintf(stderr, "max[0]=%d  max[1]=%d  max[2]=%d\n",max[0],max[1],max[2]);
        fprintf(stderr, "x0=%f  y0=%f  z0=%f\n",x0,y0,z0);
        fprintf(stderr, "nx=%d  ny=%d  nz=%d\n",nx,ny,nz);
        fprintf(stderr, "dx=%f  dy=%f  dz=%f\n",dx,dy,dz);

        Rappture::Mesh1D xgrid(x0,max[0],nx);
        Rappture::Mesh1D ygrid(y0,max[1],ny);
        Rappture::Mesh1D zgrid(z0,max[2],nz);
        Rappture::FieldRect3D field(xgrid,ygrid,zgrid);

        int ix = 0;
        int iy = 0;
        int iz = 0;
        for (int lcv=0; lcv < dataInfo.n; lcv++) {
            int nindex = iz*nx*ny + iy*nx + ix;
            fprintf(stderr, "%i    %f\n",nindex,dxdata[lcv]);
            field.define(nindex, dxdata[lcv]);
            if (++iz >= nz) {
                iz = 0;
                if (++iy >= ny) {
                    iy = 0;
                    ++ix;
                }
            }
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

        fprintf(stderr, "nx=%d  ny=%d  nz=%d\n",nx,ny,nz);
        fprintf(stderr, "dx=%f  dy=%f  dz=%f\n",dx,dy,dz);
        fprintf(stderr, "vmin = %f    dv=%f\n",vmin,dv);

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
                    fprintf(stderr, "%i    %f    %f    %f    %f    %f\n",ngen,data[ngen],field.value(xval,yval,zval),xval,yval,zval);
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
                    }

                    // gradient in y-direction
                    valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                    valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                    if (valm1 < 0 || valp1 < 0) {
                        data[ngen+2] = 0.0;
                    } else {
                        data[ngen+2] = valp1-valm1; // assume dy=1
                    }

                    // gradient in z-direction
                    valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                    valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                    if (valm1 < 0 || valp1 < 0) {
                        data[ngen+3] = 0.0;
                    } else {
                        data[ngen+3] = valp1-valm1; // assume dz=1
                    }

                    ngen += 4;
                }
            }
        }

        NanoVis::load_volume(index, nx, ny, nz, 4, data,
            field.valueMin(), field.valueMax(), nzero_min);

        delete [] data;

        float dx0 = -0.5;
        float dy0 = -0.5*dy/dx;
        float dz0 = -0.5*dz/dx;
        NanoVis::volume[index]->move(Vector3(dx0, dy0, dz0));

    }
    else {
        int rank = 3;
        int nsample = 30;
        float numPts[] = {nsample,nsample,nsample};
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
        interpolator = DXNewInterpolator(dxobj,INTERP_INIT_IMMEDIATE,-1.0);
        DXInterpolate(interpolator,&interppts,interppos,result);
        fprintf(stderr,"interppts = %i\n",interppts);
        for (int lcv = 0, pt = 0; lcv < pts; lcv+=3,pt+=9) {
            fprintf(stderr,
                    "(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n",
                    interppos[pt],interppos[pt+1],interppos[pt+2], result[lcv],
                    interppos[pt+3],interppos[pt+4],interppos[pt+5],result[lcv+1],
                    interppos[pt+6],interppos[pt+7],interppos[pt+8],result[lcv+2]);
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

        fprintf(stderr, "dx = %f    dy = %f    dz = %f\n",dx,dy,dz);
        fprintf(stderr, "dmin = %f\n",dmin);
        nx = (int)ceil(dx/dmin);
        ny = (int)ceil(dy/dmin);
        nz = (int)ceil(dz/dmin);

        fprintf(stderr, "nx = %i    ny = %i    nz = %i\n",nx,ny,nz);
#ifndef NV40
        // must be an even power of 2 for older cards
        nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
        ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
        nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

        /*
        int sub[3] = {2,0,-2};

        fprintf(stderr, "nx = %i    ny = %i    nz = %i\n",nx,ny,nz);
        float *data = new float[4*nx*ny*nz];

        for (int i=0,j=0; i<(4*nx*ny*nz); i+=4,j+=1) {
            // scale all values [0,1], -1 => out of bounds
            // data[i] = (isnan(result[j]])) ? -1.0 :
            data[i] = (isnan(result[j])) ? -1.0 :
                        (result[j] - dataMin)/(dataMax-dataMin);
            data[i+1] = -1.0;
            data[i+2] = -1.0;
            data[i+3] = -1.0;

            fprintf(stderr, "data[%i] = % 8e    data[%i] = % 8e    data[%i] = % 8e    data[%i] = % 8e\n",i,data[i],i+1,data[i+1],i+2,data[i+2],i+3,data[i+3]);
        }
        */

        fprintf(stderr, "nx = %i    ny = %i    nz = %i\n",nx,ny,nz);
        float *data = new float[nx*ny*nz];
        for (int i=0; i<(nx*ny*nz); i+=1) {
            // scale all values [0,1], -1 => out of bounds
            data[i] = (isnan(result[i])) ? -1.0 :
                        (result[i] - dataMin)/(dataMax-dataMin);
            fprintf(stderr, "data[%i] = % 8e\n",i,data[i]);
        }

        /*
        fprintf(stderr, "nx = %i    ny = %i    nz = %i\n",nx,ny,nz);
        float *data = new float[nx*ny*nz];
        int axislen[] = {nx,ny,nz};
        int xyz[] = {0,0,0};
        int idxr = 0;
        int idxd = 0;
        for (int x=0; x<nx; x++) {
            for (int y=0; y<ny; y++) {
                for (int z=0; z<nz; z++) {
                    idxd = 0;
                    // xyz = [x,y,z];
                    xyz[0] = x;
                    xyz[1] = y;
                    xyz[2] = z;
                    xyz2idx(axislen,xyz,&idxd);

                    // scale all values [0,1], -1 => out of bounds
                    data[idxd] = (isnan(result[idxr])) ? -1.0 :
                                    (result[idxr] - dataMin)/(dataMax-dataMin);
                    fprintf(stderr, "data[%i/%i,%i,%i] = % 8e\n",idxd,x,y,z,data[idxd]);
                    idxr++;
                }
            }
        }
        */

        /*
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
                    }

                    // gradient in y-direction
                    valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                    valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                    if (valm1 < 0 || valp1 < 0) {
                        data[ngen+2] = 0.0;
                    } else {
                        data[ngen+2] = valp1-valm1; // assume dy=1
                    }

                    // gradient in z-direction
                    valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                    valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                    if (valm1 < 0 || valp1 < 0) {
                        data[ngen+3] = 0.0;
                    } else {
                        data[ngen+3] = valp1-valm1; // assume dz=1
                    }

                    ngen += 4;
                }
            }
        }
        */

        NanoVis::load_volume(index, nx, ny, nz, 1, data, dataMin, dataMax, nzero_min);

        float dx0 = -0.5;
        float dy0 = -0.5*dy/dx;
        float dz0 = -0.5*dz/dx;
        NanoVis::volume[index]->move(Vector3(dx0, dy0, dz0));
    }

    return result;
}

