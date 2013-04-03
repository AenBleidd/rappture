/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_CONTOUR3D_H
#define VTKVIS_CONTOUR3D_H

#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>

#include <vector>

#include "ColorMap.h"
#include "GraphicsObject.h"

namespace VtkVis {

/**
 * \brief 3D Contour isosurfaces (geometry)
 */
class Contour3D : public GraphicsObject {
public:
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };

    Contour3D(int numContours);

    Contour3D(const std::vector<double>& contours);

    virtual ~Contour3D();

    virtual const char *getClassName() const
    {
        return "Contour3D";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setNumContours(int numContours);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setContourField(const char *name);

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
    Contour3D();

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
    vtkSmartPointer<vtkPolyDataNormals> _normalsGenerator;
    vtkSmartPointer<vtkPolyDataMapper> _dsMapper;
};

}

#endif
