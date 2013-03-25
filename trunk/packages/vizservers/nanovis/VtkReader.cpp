/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */

#include <iostream>
#include <float.h>

#include <RpOutcome.h>
#include <RpField1D.h>
#include <RpFieldRect3D.h>

#include <vrmath/Vector3f.h>

#include "ReaderCommon.h"

#include "nanovis.h"
#include "VtkReader.h"
#include "Volume.h"
#include "Trace.h"

enum FieldType {
    FLOAT,
    DOUBLE,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT
};

static inline int typeSize(FieldType type)
{
    switch (type) {
    case FLOAT:
        return sizeof(float);
    case DOUBLE:
        return sizeof(double);
    case UCHAR:
        return sizeof(unsigned char);
    case SHORT:
        return sizeof(short);
    case USHORT:
        return sizeof(unsigned short);
    case INT:
        return sizeof(int);
    case UINT:
        return sizeof(unsigned int);
    default:
        return 0;
    }
}

static inline void allocFieldData(void **data, int npts, FieldType type)
{
    switch (type) {
    case FLOAT:
        *data = (float *)(new float[npts]);
        break;
    case DOUBLE:
        *data = (double *)(new double[npts]);
        break;
    case UCHAR:
        *data = (unsigned char *)(new unsigned char[npts]);
        break;
    case SHORT:
        *data = (short *)(new short[npts]);
        break;
    case USHORT:
        *data = (unsigned short *)(new unsigned short[npts]);
        break;
    case INT:
        *data = (int *)(new int[npts]);
        break;
    case UINT:
        *data = (unsigned int *)(new unsigned int[npts]);
        break;
    default:
        ;
    }
}

static inline void readFieldValue(void *data, int idx, FieldType type, std::iostream& fin)
{
    switch (type) {
    case FLOAT: {
        float val;
        fin >> val;
        ((float *)data)[idx] = val;
    }
        break;
    case DOUBLE: {
        double val;
        fin >> val;
        ((double *)data)[idx] = val;
    }
        break;
    case SHORT: {
        short val;
        fin >> val;
        ((short *)data)[idx] = val;
    }
        break;
    case USHORT: {
        unsigned short val;
        fin >> val;
        ((unsigned short *)data)[idx] = val;
    }
        break;
    case INT: {
        int val;
        fin >> val;
        ((int *)data)[idx] = val;
    }
        break;
    case UINT: {
        unsigned int val;
        fin >> val;
        ((unsigned int *)data)[idx] = val;
    }
        break;
    default:
        break;
    }

}

static inline float getFieldData(void *data, int i, FieldType type)
{
    switch (type) {
    case FLOAT:
        return ((float *)data)[i];
    case DOUBLE:
        return (float)((double *)data)[i];
    case UCHAR:
        return ((float)((unsigned char *)data)[i]);
    case SHORT:
        return (float)((short *)data)[i];
    case USHORT:
        return (float)((unsigned short *)data)[i];
    case INT:
        return (float)((int *)data)[i];
    case UINT:
        return (float)((unsigned int *)data)[i];
    default:
        return 0.0f;
    }
}

static inline void deleteFieldData(void *data, FieldType type)
{
    if (data == NULL)
        return;
    switch (type) {
    case FLOAT:
        delete [] ((float *)data);
        break;
    case DOUBLE:
        delete [] ((double *)data);
        break;
    case UCHAR:
        delete [] ((unsigned char *)data);
        break;
    case SHORT:
        delete [] ((short *)data);
        break;
    case USHORT:
        delete [] ((unsigned short *)data);
        break;
    case INT:
        delete [] ((int *)data);
        break;
    case UINT:
        delete [] ((unsigned int *)data);
        break;
    default:
        ;
    }
}

Volume *
load_vtk_volume_stream(Rappture::Outcome& result, const char *tag, std::iostream& fin)
{
    TRACE("Enter tag:%s", tag);

    int nx, ny, nz, npts;
    // origin
    double x0, y0, z0;
    // deltas (cell dimensions)
    double dx, dy, dz;
    // length of volume edges
    double lx, ly, lz;
    char line[128], str[128], type[128], *start;
    int isBinary = -1;
    int isRect = -1;
    FieldType ftype = DOUBLE;
    void *fieldData = NULL;
    int numRead = 0;

    dx = dy = dz = 0.0;
    x0 = y0 = z0 = 0.0;
    nx = ny = nz = npts = 0;
    while (!fin.eof()) {
        fin.getline(line, sizeof(line) - 1);
        if (fin.fail()) {
            result.error("Error in data stream");
            return NULL;
        }
        for (start = line; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (strncmp(start, "BINARY", 6) == 0) {
                isBinary = 1;
            } else if (strncmp(start, "ASCII", 5) == 0) {
                isBinary = 0;
            } else if (sscanf(start, "DATASET %s", str) == 1) {
                if (strncmp(str, "STRUCTURED_POINTS", 17) == 0) {
                    isRect = 1;
                } else {
                    result.addError("Unsupported DataSet type '%s'", str);
                    return NULL;
                }
            } else if (sscanf(start, "DIMENSIONS %d %d %d", &nx, &ny, &nz) == 3) {
                if (nx <= 0 || ny <= 0 || nz <= 0) {
                    result.error("Found non-positive dimensions");
                    return NULL;
                }
                // found dimensions
            } else if (sscanf(start, "ORIGIN %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            } else if (sscanf(start, "SPACING %lg %lg %lg", &dx, &dy, &dz) == 3) {
                if ((nx > 1 && dx == 0.0) || (ny > 1 && dy == 0.0) || (nz > 1 && dz == 0.0)) {
                    result.error("Found zero spacing for dimension");
                    return NULL;
                }
                // found cell spacing
            } else if (sscanf(start, "POINT_DATA %d", &npts) == 1) {
                if (npts < 1 || npts != nx * ny * nz) {
                    result.addError("Error in data stream: unexpected number of point data: %d", npts);
                    return NULL;
                }
                if (isBinary < 0 || fin.eof()) {
                    // Should know if ASCII or BINARY by now
                    result.error("Error in data stream");
                    return NULL;
                }
                fin.getline(line, sizeof(line) - 1);
                if (fin.fail()) {
                    result.error("Error in data stream");
                    return NULL;
                }
                for (start = line; *start == ' ' || *start == '\t'; start++)
                    ;  // skip leading blanks
                if (sscanf(start, "SCALARS %s %s", str, type) != 2) {
                    result.error("Error in data stream");
                    return NULL;
                }
                if (strncmp(type, "double", 6) == 0) {
                    ftype = DOUBLE;
                } else if (strncmp(type, "float", 5) == 0) {
                    ftype = FLOAT;
                } else if (strncmp(type, "unsigned_char", 13) == 0) {
                    ftype = UCHAR;
                    if (!isBinary) {
                        result.addError("unsigned char only supported in binary VTK files");
                        return NULL;
                    }
                } else if (strncmp(type, "short", 5) == 0) {
                    ftype = SHORT;
                } else if (strncmp(type, "unsigned_short", 14) == 0) {
                    ftype = USHORT;
                } else if (strncmp(type, "int", 3) == 0) {
                    ftype = INT;
                } else if (strncmp(type, "unsigned_int", 12) == 0) {
                    ftype = UINT;
                } else {
                    result.addError("Unsupported scalar type: '%s'", type);
                    return NULL;
                }
                if (fin.eof()) {
                    result.error("Error in data stream");
                    return NULL;
                }
                fin.getline(line, sizeof(line) - 1);
                if (fin.fail()) {
                    result.error("Error in data stream");
                    return NULL;
                }
                for (start = line; *start == ' ' || *start == '\t'; start++)
                    ;  // skip leading blanks
                if (sscanf(start, "LOOKUP_TABLE %s", str) == 1) {
                    // skip lookup table, but don't read ahead
                    if (fin.eof()) {
                        result.error("Error in data stream");
                        return NULL;
                    }
                } else {
                    // Lookup table line is required
                    result.error("Missing LOOKUP_TABLE");
                    return NULL;
                }
                // found start of field data
                allocFieldData(&fieldData, npts, ftype);
                if (isBinary) {
                    fin.read((char *)fieldData, npts * typeSize(ftype));
                    if (fin.fail()) {
                        deleteFieldData(fieldData, ftype);
                        result.error("Error in data stream");
                        return NULL;
                    }
                    numRead = npts;
                } else {
                    numRead = 0;
                    while (!fin.eof() && numRead < npts) {
                        readFieldValue(fieldData, numRead++, ftype, fin);
                        if (fin.fail()) {
                            deleteFieldData(fieldData, ftype);
                            result.error("Error in data stream");
                            return NULL;
                        }
                    }
                }
                
                break;
            }
        }
    }

    if (isRect < 0 || numRead != npts || npts < 1) {
        deleteFieldData(fieldData, ftype);
        result.error("Error in data stream");
        return NULL;
    }

    TRACE("found nx=%d ny=%d nz=%d\ndx=%f dy=%f dz=%f\nx0=%f y0=%f z0=%f", 
          nx, ny, nz, dx, dy, dz, x0, y0, z0);

    lx = (nx - 1) * dx;
    ly = (ny - 1) * dy;
    lz = (nz - 1) * dz;

    Volume *volPtr = NULL;
    double vmin = DBL_MAX;
    double nzero_min = DBL_MAX;
    double vmax = -DBL_MAX;
    float *data = NULL;

    if (isRect) {
#ifdef DOWNSAMPLE_DATA
        Rappture::Mesh1D xgrid(x0, x0 + lx, nx);
        Rappture::Mesh1D ygrid(y0, y0 + ly, ny);
        Rappture::Mesh1D zgrid(z0, z0 + lz, nz);
        Rappture::FieldRect3D field(xgrid, ygrid, zgrid);
#else // !DOWNSAMPLE_DATA
        data = new float[npts * 4];
        memset(data, 0, npts * 4);
#endif // DOWNSAMPLE_DATA
        for (int p = 0; p < npts; p++) {
#ifdef DOWNSAMPLE_DATA
            field.define(p, getFieldData(fieldData, p, ftype));
#else // !DOWNSAMPLE_DATA
            int nindex = p * 4;
            data[nindex] = getFieldData(fieldData, p, ftype);
            if (data[nindex] < vmin) {
                vmin = data[nindex];
            } else if (data[nindex] > vmax) {
                vmax = data[nindex];
            }
            if (data[nindex] != 0.0 && data[nindex] < nzero_min) {
                nzero_min = data[nindex];
            }
#endif // DOWNSAMPLE_DATA
        }

        deleteFieldData(fieldData, ftype);

#ifndef DOWNSAMPLE_DATA
        double dv = vmax - vmin;
        if (dv == 0.0) {
            dv = 1.0;
        }

        int ngen = 0;
        const int step = 4;
        for (int i = 0; i < npts; ++i) {
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
        float *cdata = new float[npts * step];
#else // !FILTER_GRADIENTS
        // Leave 3 floats of space for gradients
        // to be filled in by computeSimpleGradient()
        const int step = 4;
        data = new float[npts * step];
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
        deleteFieldData(fieldData, ftype);
        result.error("Unsupported DataSet");
        return NULL;
    }

    TRACE("nx = %i ny = %i nz = %i", nx, ny, nz);
    TRACE("x0 = %lg y0 = %lg z0 = %lg", x0, y0, z0);
    TRACE("lx = %lg ly = %lg lz = %lg", lx, ly, lz);
    TRACE("dx = %lg dy = %lg dz = %lg", dx, dy, dz);
    TRACE("dataMin = %lg dataMax = %lg nzero_min = %lg",
          vmin, vmax, nzero_min);

    volPtr = NanoVis::loadVolume(tag, nx, ny, nz, 4, data,
                                 vmin, vmax, nzero_min);
    volPtr->xAxis.setRange(x0, x0 + lx);
    volPtr->yAxis.setRange(y0, y0 + ly);
    volPtr->zAxis.setRange(z0, z0 + lz);
    volPtr->updatePending = true;

    delete [] data;

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*ly/lx;
    float dz0 = -0.5*lz/lx;
    if (volPtr) {
        volPtr->location(vrmath::Vector3f(dx0, dy0, dz0));
        TRACE("Set volume location to %g %g %g", dx0, dy0, dz0);
    }
    return volPtr;
}
