/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */

#include <iostream>
#include <float.h>

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkProbeFilter.h>
#include <vtkCutter.h>
#include <vtkImageData.h>
#include <vtkImageResample.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkTransform.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkShepardMethod.h>

#include "DataSetResample.h"
#include "Trace.h"

enum PrincipalPlane {
    PLANE_XY,
    PLANE_ZY,
    PLANE_XZ
};

static bool is2D(vtkDataSet *dataSet, PrincipalPlane *plane, double *offset)
{
    double bounds[6];
    dataSet->GetBounds(bounds);
    if (bounds[4] == bounds[5]) {
        // Zmin = Zmax, XY plane
        if (plane != NULL) {
            *plane = PLANE_XY;
        }
        if (offset != NULL)
            *offset = bounds[4];
        return true;
    } else if (bounds[0] == bounds[1]) {
        // Xmin = Xmax, ZY plane
        if (plane != NULL) {
            *plane = PLANE_ZY;
        }
        if (offset != NULL)
            *offset = bounds[0];
        return true;
    } else if (bounds[2] == bounds[3]) {
        // Ymin = Ymax, XZ plane
        if (plane != NULL) {
            *plane = PLANE_XZ;
         }
        if (offset != NULL)
            *offset = bounds[2];
        return true;
    }
    return false;
}

/**
 * \brief Determines if mesh is a point cloud (has no cells)
 */
static bool isCloud(vtkDataSet *dataSet)
{
    vtkPolyData *pd = vtkPolyData::SafeDownCast(dataSet);
    if (pd) {
        // If PolyData, is a cloud if there are no cells other than vertices
        if (pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            return true;
        } else {
            // Has cells
            return false;
        }
    } else {
        return (dataSet->GetNumberOfCells() == 0);
    }
}

vtkSmartPointer<vtkImageData>
nv::resampleVTKDataSet(vtkDataSet *dataSetIn, int maxDim, nv::CloudStyle cloudStyle)
{
    double bounds[6];
    dataSetIn->GetBounds(bounds);

    int xdim, ydim, zdim;
    double xSpacing, ySpacing, zSpacing;

    vtkSmartPointer<vtkImageData> dataSetOut;
    if (vtkImageData::SafeDownCast(dataSetIn) != NULL) {
        // We have a uniform grid, use vtkImageResample
        vtkImageData *imgIn = vtkImageData::SafeDownCast(dataSetIn);

        TRACE("Image input Dims: %d,%d,%d Spacing: %g,%g,%g",
              imgIn->GetDimensions()[0],
              imgIn->GetDimensions()[1],
              imgIn->GetDimensions()[2],
              imgIn->GetSpacing()[0],
              imgIn->GetSpacing()[1],
              imgIn->GetSpacing()[2]);

        // Input DataSet is vtkImageData
        xdim = (imgIn->GetDimensions()[0] > maxDim) ? maxDim : imgIn->GetDimensions()[0];
        ydim = (imgIn->GetDimensions()[1] > maxDim) ? maxDim : imgIn->GetDimensions()[1];
        zdim = (imgIn->GetDimensions()[2] > maxDim) ? maxDim : imgIn->GetDimensions()[2];

        xSpacing = (bounds[1]-bounds[0])/((double)(xdim-1));
        ySpacing = (bounds[3]-bounds[2])/((double)(ydim-1));
        zSpacing = (bounds[5]-bounds[4])/((double)(zdim-1));

        vtkSmartPointer<vtkImageResample> filter = vtkSmartPointer<vtkImageResample>::New();
        filter->SetInputData(dataSetIn);
        filter->SetAxisOutputSpacing(0, xSpacing);
        filter->SetAxisOutputSpacing(1, ySpacing);
        filter->SetAxisOutputSpacing(2, zSpacing);
        filter->Update();
        dataSetOut = filter->GetOutput();
    } else {
        double xLen = bounds[1] - bounds[0];
        double yLen = bounds[3] - bounds[2];
        double zLen = bounds[5] - bounds[4];

        xdim = (xLen == 0.0 ? 1 : maxDim);
        ydim = (yLen == 0.0 ? 1 : maxDim);
        zdim = (zLen == 0.0 ? 1 : maxDim);

        xSpacing = (xLen == 0.0 ? 0.0 : xLen/((double)(xdim-1)));
        ySpacing = (yLen == 0.0 ? 0.0 : yLen/((double)(ydim-1)));
        zSpacing = (zLen == 0.0 ? 0.0 : zLen/((double)(zdim-1)));

        vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
        imageData->SetDimensions(xdim, ydim, zdim);
        imageData->SetOrigin(bounds[0], bounds[2], bounds[4]);
        imageData->SetSpacing(xSpacing, ySpacing, zSpacing); 

        vtkSmartPointer<vtkProbeFilter> filter = vtkSmartPointer<vtkProbeFilter>::New();
        filter->SetInputData(imageData);

        if (isCloud(dataSetIn)) {
            // Need to mesh or splat cloud
            if (cloudStyle == CLOUD_MESH) {
                PrincipalPlane plane;
                double offset;
                if (is2D(dataSetIn, &plane, &offset)) {
                    vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                    if (plane == PLANE_ZY) {
                        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                        trans->RotateWXYZ(90, 0, 1, 0);
                        if (offset != 0.0) {
                            trans->Translate(-offset, 0, 0);
                        }
                        mesher->SetTransform(trans);
                    } else if (plane == PLANE_XZ) {
                        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                        trans->RotateWXYZ(-90, 1, 0, 0);
                        if (offset != 0.0) {
                            trans->Translate(0, -offset, 0);
                        }
                        mesher->SetTransform(trans);
                    } else if (offset != 0.0) {
                        // XY with Z offset
                        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                        trans->Translate(0, 0, -offset);
                        mesher->SetTransform(trans);
                    }
                    mesher->SetInputData(dataSetIn);
                    filter->SetSourceConnection(mesher->GetOutputPort());
                    //filter->SpatialMatchOn();
                    filter->Update();
                    dataSetOut = filter->GetImageDataOutput();
                } else {
                    vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                    mesher->SetInputData(dataSetIn);
                    filter->SetSourceConnection(mesher->GetOutputPort());
                    //filter->SpatialMatchOn();
                    filter->Update();
                    dataSetOut = filter->GetImageDataOutput();
                }
            } else if (cloudStyle == CLOUD_SPLAT) {
                // CLOUD_SPLAT
                vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                splatter->SetInputData(dataSetIn);
                PrincipalPlane plane;
                double offset;
                int dims[3];
                dims[0] = xdim;
                dims[1] = ydim;
                dims[2] = zdim;
                if (is2D(dataSetIn, &plane, &offset)) {
                    vtkSmartPointer<vtkExtractVOI> volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
                    if (plane == PLANE_ZY) {
                        dims[0] = 3;
                        volumeSlicer->SetVOI(1, 1, 0, dims[1]-1, 0, dims[1]-1);
                    } else if (plane == PLANE_XZ) {
                        dims[1] = 3;
                        volumeSlicer->SetVOI(0, dims[0]-1, 1, 1, 0, dims[2]-1);
                    } else {
                        dims[2] = 3;
                        volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                    }
                    splatter->SetSampleDimensions(dims);
                    volumeSlicer->SetInputConnection(splatter->GetOutputPort());
                    volumeSlicer->SetSampleRate(1, 1, 1);
                    volumeSlicer->Update();
                    dataSetOut = volumeSlicer->GetOutput();
                } else {
                    splatter->SetSampleDimensions(dims);
                    splatter->Update();
                    dataSetOut = splatter->GetOutput();
                }
            } else {
                // CLOUD_SHEAPRD_METHOD
                vtkSmartPointer<vtkShepardMethod> splatter = vtkSmartPointer<vtkShepardMethod>::New();
                splatter->SetInputData(dataSetIn);
                PrincipalPlane plane;
                double offset;
                int dims[3];
                dims[0] = xdim;
                dims[1] = ydim;
                dims[2] = zdim;
                if (is2D(dataSetIn, &plane, &offset)) {
                    vtkSmartPointer<vtkExtractVOI> volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
                    if (plane == PLANE_ZY) {
                        dims[0] = 3;
                        volumeSlicer->SetVOI(1, 1, 0, dims[1]-1, 0, dims[1]-1);
                    } else if (plane == PLANE_XZ) {
                        dims[1] = 3;
                        volumeSlicer->SetVOI(0, dims[0]-1, 1, 1, 0, dims[2]-1);
                    } else {
                        dims[2] = 3;
                        volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                    }
                    splatter->SetSampleDimensions(dims);
                    volumeSlicer->SetInputConnection(splatter->GetOutputPort());
                    volumeSlicer->SetSampleRate(1, 1, 1);
                    volumeSlicer->Update();
                    dataSetOut = volumeSlicer->GetOutput();
                } else {
                    splatter->SetSampleDimensions(dims);
                    splatter->Update();
                    dataSetOut = splatter->GetOutput();
                }
            }
        } else {
            filter->SetSourceData(dataSetIn);
            //filter->SpatialMatchOn();
            filter->Update();
            dataSetOut = filter->GetImageDataOutput();
        }
    }

    TRACE("Image output Dims: %d,%d,%d Spacing: %g,%g,%g",
          dataSetOut->GetDimensions()[0],
          dataSetOut->GetDimensions()[1],
          dataSetOut->GetDimensions()[2],
          dataSetOut->GetSpacing()[0],
          dataSetOut->GetSpacing()[1],
          dataSetOut->GetSpacing()[2]);

    return dataSetOut;
}
