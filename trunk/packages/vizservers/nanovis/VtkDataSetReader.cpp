/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */

#include <iostream>
#include <float.h>

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkDataSetReader.h>
#include <vtkCharArray.h>

#include <RpOutcome.h>

#include <vrmath/Vector3f.h>

#include "ReaderCommon.h"
#include "DataSetResample.h"

#include "config.h"
#include "nanovis.h"
#include "VtkDataSetReader.h"
#include "Volume.h"
#include "Trace.h"

using namespace nv;

Volume *
nv::load_vtk_volume_stream(Rappture::Outcome& result, const char *tag, const char *bytes, int nBytes)
{
    TRACE("Enter tag:%s", tag);

    vtkSmartPointer<vtkImageData> resampledDataSet;
    {
        vtkSmartPointer<vtkDataSet> dataSet;
        {
            vtkSmartPointer<vtkDataSetReader> reader = vtkSmartPointer<vtkDataSetReader>::New();
            vtkSmartPointer<vtkCharArray> dataSetString = vtkSmartPointer<vtkCharArray>::New();

            dataSetString->SetArray(const_cast<char *>(bytes), nBytes, 1);
            reader->SetInputArray(dataSetString);
            reader->ReadFromInputStringOn();
            //reader->ReadAllNormalsOn();
            //reader->ReadAllTCoordsOn();
            reader->ReadAllScalarsOn();
            //reader->ReadAllColorScalarsOn();
            reader->ReadAllVectorsOn();
            //reader->ReadAllTensorsOn();
            //reader->ReadAllFieldsOn();
            reader->SetLookupTableName("");
            reader->Update();

            dataSet = reader->GetOutput();
        }

        if (dataSet == NULL)
            return NULL;

        int maxDim = 64;

        resampledDataSet = vtkImageData::SafeDownCast(dataSet.GetPointer());
        if (resampledDataSet != NULL) {
            // Have a uniform grid, check if we need to resample
#ifdef DOWNSAMPLE_DATA
            if (resampledDataSet->GetDimensions()[0] > maxDim ||
                resampledDataSet->GetDimensions()[1] > maxDim ||
                resampledDataSet->GetDimensions()[2] > maxDim) {
                resampledDataSet = resampleVTKDataSet(dataSet, maxDim);
            }
#endif
        } else {
            resampledDataSet = resampleVTKDataSet(dataSet, maxDim);
        }
    }

    int nx, ny, nz, npts;
    // origin
    double x0, y0, z0;
    // deltas (cell dimensions)
    double dx, dy, dz;
    // length of volume edges
    double lx, ly, lz;

    nx = resampledDataSet->GetDimensions()[0];
    ny = resampledDataSet->GetDimensions()[1];
    nz = resampledDataSet->GetDimensions()[2];
    npts = nx * ny * nz;
    resampledDataSet->GetSpacing(dx, dy, dz);
    resampledDataSet->GetOrigin(x0, y0, z0);

    lx = (nx - 1) * dx;
    ly = (ny - 1) * dy;
    lz = (nz - 1) * dz;

    Volume *volume = NULL;
    double vmin = DBL_MAX;
    double nzero_min = DBL_MAX;
    double vmax = -DBL_MAX;

    float *data = new float[npts * 4];
    memset(data, 0, npts * 4);

    bool isVectorData = (resampledDataSet->GetPointData()->GetVectors() != NULL);

    int ix = 0;
    int iy = 0;
    int iz = 0;
    for (int p = 0; p < npts; p++) {
        int nindex = p * 4;
        double val;
        if (isVectorData) {
            double vec[3];
            int loc[3];
            loc[0] = ix; loc[1] = iy; loc[2] = iz;
            vtkIdType idx = resampledDataSet->ComputePointId(loc);
            resampledDataSet->GetPointData()->GetVectors()->GetTuple(idx, vec);
            val = sqrt(vec[0]*vec[0] + vec[1]*vec[1] * vec[2]*vec[2]);
            data[nindex] = (float)val;
            data[nindex+1] = (float)vec[0];
            data[nindex+2] = (float)vec[1];
            data[nindex+3] = (float)vec[2];
        } else {
            val = resampledDataSet->GetScalarComponentAsDouble(ix, iy, iz, 0);
            data[nindex] = (float)val;
        }
         if (val < vmin) {
            vmin = val;
        } else if (val > vmax) {
            vmax = val;
        }
        if (val != 0.0 && val < nzero_min) {
            nzero_min = val;
        }

        if (++ix >= nx) {
            ix = 0;
            if (++iy >= ny) {
                iy = 0;
                ++iz;
            }
        }
    }

    if (isVectorData) {
        // Normalize magnitude [0,1] and vector components [0,1]
        normalizeVector(data, npts, vmin, vmax);
    } else {
        // scale all values [0-1], -1 => out of bounds
        normalizeScalar(data, npts, 4, vmin, vmax);
        computeSimpleGradient(data, nx, ny, nz,
                              dx, dy, dz);
    }

    TRACE("nx = %i ny = %i nz = %i", nx, ny, nz);
    TRACE("x0 = %lg y0 = %lg z0 = %lg", x0, y0, z0);
    TRACE("lx = %lg ly = %lg lz = %lg", lx, ly, lz);
    TRACE("dx = %lg dy = %lg dz = %lg", dx, dy, dz);
    TRACE("dataMin = %lg dataMax = %lg nzero_min = %lg",
          vmin, vmax, nzero_min);

    volume = NanoVis::loadVolume(tag, nx, ny, nz, 4, data,
                                 vmin, vmax, nzero_min);
    volume->xAxis.setRange(x0, x0 + lx);
    volume->yAxis.setRange(y0, y0 + ly);
    volume->zAxis.setRange(z0, z0 + lz);
    volume->updatePending = true;

    delete [] data;

    TRACE("Leave");
    return volume;
}
