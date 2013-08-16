/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_DATASET_RESAMPLE_H
#define NV_DATASET_RESAMPLE_H

#include <iostream>

#include <vtkSmartPointer.h>

class vtkDataSet;
class vtkImageData;

namespace nv {

enum CloudStyle {
    CLOUD_MESH,
    CLOUD_SPLAT,
    CLOUD_SHEPARD_METHOD
};

extern vtkSmartPointer<vtkImageData>
resampleVTKDataSet(vtkDataSet *dataSetIn, int maxDim, CloudStyle cloudStyle = CLOUD_MESH);

}

#endif
