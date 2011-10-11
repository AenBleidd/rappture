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
#include <vtkPlane.h>

#include "ColorMap.h"
#include "RpTypes.h"
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
    LIC();
    virtual ~LIC();

    virtual const char *getClassName() const
    {
        return "LIC";
    }
    
    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void selectVolumeSlice(Axis axis, double ratio);

    void setColorMap(ColorMap *colorMap);

    /**
     * \brief Return the ColorMap in use
     */
    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void updateColorMap();

    virtual void updateRanges(Renderer *renderer);

private:
    virtual void initProp();
    virtual void update();

    Axis _sliceAxis;
    ColorMap *_colorMap;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkExtractVOI> _volumeSlicer;
    vtkSmartPointer<vtkProbeFilter> _probeFilter;
    vtkSmartPointer<vtkPlane> _cutPlane;
    vtkSmartPointer<vtkImageDataLIC2D> _lic;
    vtkSmartPointer<vtkSurfaceLICPainter> _painter;
    vtkSmartPointer<vtkMapper> _mapper;
};

}
}

#endif
