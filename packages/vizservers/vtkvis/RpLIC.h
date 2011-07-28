/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_LIC_H__
#define __RAPPTURE_VTKVIS_LIC_H__

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkPlaneCollection.h>
#include <vtkExtractVOI.h>
#include <vtkProbeFilter.h>
#include <vtkImageDataLIC2D.h>
#include <vtkSurfaceLICPainter.h>
#include <vtkMapper.h>
#include <vtkLookupTable.h>

#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Line Integral Convolution visualization of vector fields
 */
class LIC {
public:
    enum Axis {
        X_AXIS,
        Y_AXIS,
        Z_AXIS
    };

    LIC();
    virtual ~LIC();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void selectVolumeSlice(Axis axis, double ratio);

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setOpacity(double opacity);

    double getOpacity();

    void setVisibility(bool state);

    bool getVisibility();

    void setEdgeVisibility(bool state);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    void initProp();
    void update();

    DataSet *_dataSet;

    float _edgeColor[3];
    float _edgeWidth;
    double _opacity;
    bool _lighting;
    Axis _sliceAxis;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkActor> _prop;
    vtkSmartPointer<vtkExtractVOI> _volumeSlicer;
    vtkSmartPointer<vtkProbeFilter> _probeFilter;
    vtkSmartPointer<vtkImageDataLIC2D> _lic;
    vtkSmartPointer<vtkSurfaceLICPainter> _painter;
    vtkSmartPointer<vtkMapper> _mapper;
};

}
}

#endif
