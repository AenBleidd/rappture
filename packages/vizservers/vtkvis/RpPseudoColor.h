/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_PSEUDOCOLOR_H__
#define __RAPPTURE_VTKVIS_PSEUDOCOLOR_H__

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>

#include "ColorMap.h"
#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Color-mapped plot of data set
 * 
 * Currently the DataSet must be image data (2D uniform grid)
 */
class PseudoColor : public VtkGraphicsObject {
public:
    PseudoColor();
    virtual ~PseudoColor();

    virtual const char *getClassName() const
    {
        return "PseudoColor";
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setColorMap(ColorMap *colorMap);

    /**
     * \brief Return the ColorMap in use
     */
    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void updateColorMap();

    virtual void updateRanges(bool useCumulative,
                              double scalarRange[2],
                              double vectorMagnitudeRange[2],
                              double vectorComponentRange[3][2]);

private:
    virtual void initProp();
    virtual void update();

    ColorMap *_colorMap;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
};

}
}

#endif
