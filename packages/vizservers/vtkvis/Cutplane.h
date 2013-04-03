/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_CUTPLANE_H
#define VTKVIS_CUTPLANE_H

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkOutlineFilter.h>
#include <vtkOutlineSource.h>
#include <vtkGaussianSplatter.h>

#include "ColorMap.h"
#include "Types.h"
#include "GraphicsObject.h"

namespace VtkVis {

//#define CUTPLANE_TIGHT_OUTLINE

/**
 * \brief Color-mapped plot of slice through a data set
 * 
 * Currently the DataSet must be image data (2D uniform grid)
 */
class Cutplane : public GraphicsObject {
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

    virtual void setColor(float color[3]);

    virtual void setLighting(bool state);

    virtual void setEdgeVisibility(bool state);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setOutlineVisibility(bool state);

    void selectVolumeSlice(Axis axis, double ratio);

    void setSliceVisibility(Axis axis, bool state);

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
    virtual void initProp();
    virtual void update();

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
    vtkSmartPointer<vtkActor> _actor[3];
    vtkSmartPointer<vtkActor> _borderActor[3];
    vtkSmartPointer<vtkDataSetMapper> _mapper[3];
    vtkSmartPointer<vtkPolyDataMapper> _borderMapper[3];
    vtkSmartPointer<vtkCutter> _cutter[3];
    vtkSmartPointer<vtkPlane> _cutPlane[3];
#ifdef CUTPLANE_TIGHT_OUTLINE
    vtkSmartPointer<vtkOutlineFilter> _outlineFilter[3];
#else
    vtkSmartPointer<vtkOutlineSource> _outlineSource[3];
#endif
    vtkSmartPointer<vtkGaussianSplatter> _splatter;
};

}

#endif
