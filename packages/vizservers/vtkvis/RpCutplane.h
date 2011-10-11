/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_CUTPLANE_H__
#define __RAPPTURE_VTKVIS_CUTPLANE_H__

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>
#include <vtkCutter.h>
#include <vtkPlane.h>

#include "ColorMap.h"
#include "RpTypes.h"
#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Color-mapped plot of slice through a data set
 * 
 * Currently the DataSet must be image data (2D uniform grid)
 */
class Cutplane : public VtkGraphicsObject {
public:
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z
    };

    Cutplane();
    virtual ~Cutplane();

    virtual const char *getClassName() const
    {
        return "Cutplane";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void selectVolumeSlice(Axis axis, double ratio);

    void setSliceVisibility(Axis axis, bool state);

    void setColorMode(ColorMode mode, DataSet::DataAttributeType type,
                      const char *name, double range[2] = NULL);

    void setColorMode(ColorMode mode,
                      const char *name, double range[2] = NULL);

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

    virtual void updateRanges(Renderer *renderer);

private:
    virtual void initProp();
    virtual void update();

    ColorMap *_colorMap;
    ColorMode _colorMode;
    std::string _colorFieldName;
    DataSet::DataAttributeType _colorFieldType;
    double _colorFieldRange[2];
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    Renderer *_renderer;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkActor> _actor[3];
    vtkSmartPointer<vtkDataSetMapper> _mapper[3];
    vtkSmartPointer<vtkCutter> _cutter[3];
    vtkSmartPointer<vtkPlane> _cutPlane[3];
};

}
}

#endif
