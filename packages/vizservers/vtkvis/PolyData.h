/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_POLYDATA_H
#define VTKVIS_POLYDATA_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkLookupTable.h>

#include "ColorMap.h"
#include "GraphicsObject.h"

namespace VtkVis {

/**
 * \brief VTK Mesh (Polygon data)
 *
 * This class creates a boundary mesh of a DataSet
 */
class PolyData : public GraphicsObject {
public:
    enum CloudStyle {
        CLOUD_MESH,
        CLOUD_POINTS
    };
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };

    PolyData();
    virtual ~PolyData();

    virtual const char *getClassName() const
    {
        return "PolyData";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setCloudStyle(CloudStyle style);

    void setInterpolateBeforeMapping(bool state);

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
    virtual void update();

    ColorMap *_colorMap;
    ColorMode _colorMode;
    std::string _colorFieldName;
    DataSet::DataAttributeType _colorFieldType;
    double _colorFieldRange[2];
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    Renderer *_renderer;

    CloudStyle _cloudStyle;
    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkPolyDataMapper> _mapper;
};

}

#endif
