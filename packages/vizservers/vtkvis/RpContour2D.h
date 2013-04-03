/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_CONTOUR2D_H
#define VTKVIS_CONTOUR2D_H

#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>
#include <vtkLookupTable.h>

#include <vector>

#include "ColorMap.h"
#include "RpVtkGraphicsObject.h"

namespace VtkVis {

/**
 * \brief 2D Contour lines (isolines)
 */
class Contour2D : public VtkGraphicsObject {
public:
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };

    Contour2D(int numContours);

    Contour2D(const std::vector<double>& contours);

    virtual ~Contour2D();

    virtual const char *getClassName() const
    {
        return "Contour2D";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setContourField(const char *name);

    void setNumContours(int numContours);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

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

    virtual void setColor(float color[3])
    {
        VtkGraphicsObject::setColor(color);
        VtkGraphicsObject::setEdgeColor(color);
    }

    virtual void setEdgeColor(float color[3])
    {
        setColor(color);
    }

private:
    Contour2D();

    virtual void update();

    int _numContours;
    std::vector<double> _contours;

    bool _pipelineInitialized;

    ColorMap *_colorMap;
    ColorMode _colorMode;
    std::string _colorFieldName;
    DataSet::DataAttributeType _colorFieldType;
    double _colorFieldRange[2];
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    Renderer *_renderer;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkPolyDataMapper> _dsMapper;
};

}

#endif
