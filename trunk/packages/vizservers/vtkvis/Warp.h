/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_WARP_H
#define VTKVIS_WARP_H

#include <vtkSmartPointer.h>
#include <vtkAlgorithmOutput.h>
#include <vtkLookupTable.h>
#include <vtkDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>
#include <vtkWarpVector.h>

#include "ColorMap.h"
#include "VtkGraphicsObject.h"

namespace VtkVis {

/**
 * \brief Warp a mesh based on point data vectors
 *
 * This class can be used to visualize mechanical deformation or 
 * to create flow profiles/surfaces
 */
class Warp : public VtkGraphicsObject {
public:
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };

    Warp();
    virtual ~Warp();

    virtual const char *getClassName() const
    {
        return "Warp";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setWarpScale(double scale);

    double getWarpScale()
    {
        return _warpScale;
    }

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

    vtkAlgorithmOutput *initWarp(vtkAlgorithmOutput *input);
    vtkAlgorithmOutput *initWarp(vtkDataSet *input);

    double _warpScale;

    ColorMap *_colorMap;
    ColorMode _colorMode;
    std::string _colorFieldName;
    DataSet::DataAttributeType _colorFieldType;
    double _colorFieldRange[2];
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    Renderer *_renderer;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkWarpVector> _warp;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
};

}

#endif
