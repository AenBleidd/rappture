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

#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Line Integral Convolution visualization of vector fields
 *
 *  The DataSet must contain vectors
 */
class LIC : public VtkGraphicsObject {
public:
    enum Axis {
        X_AXIS,
        Y_AXIS,
        Z_AXIS
    };

    LIC();
    virtual ~LIC();

    virtual const char *getClassName() const
    {
        return "LIC";
    }
    
    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void selectVolumeSlice(Axis axis, double ratio);

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

private:
    virtual void initProp();
    virtual void update();

    Axis _sliceAxis;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkExtractVOI> _volumeSlicer;
    vtkSmartPointer<vtkProbeFilter> _probeFilter;
    vtkSmartPointer<vtkImageDataLIC2D> _lic;
    vtkSmartPointer<vtkSurfaceLICPainter> _painter;
    vtkSmartPointer<vtkMapper> _mapper;
};

}
}

#endif
