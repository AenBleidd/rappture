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
 */
class PseudoColor : public VtkGraphicsObject {
public:
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };

    PseudoColor();
    virtual ~PseudoColor();

    virtual const char *getClassName() const
    {
        return "PseudoColor";
    }

    virtual void setDataSet(DataSet *dataSet,
                            bool useCumulative,
                            double scalarRange[2],
                            double vectorMagnitudeRange[2],
                            double vectorComponentRange[3][2]);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setColorMode(ColorMode mode);

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
    virtual void update();

    ColorMode _colorMode;
    ColorMap *_colorMap;
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
};

}
}

#endif
