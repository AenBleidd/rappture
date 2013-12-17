/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_IMAGE_CUTPLANE_H
#define VTKVIS_IMAGE_CUTPLANE_H

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkImageSlice.h>
#include <vtkImageResliceMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>
#include <vtkPlane.h>
#include <vtkOutlineSource.h>

#include "ColorMap.h"
#include "Types.h"
#include "GraphicsObject.h"

namespace VtkVis {

/**
 * \brief Color-mapped plot of orthogonal slices through a data set
 * 
 * The DataSet must be image data (3D uniform grid)
 */
class ImageCutplane : public GraphicsObject {
public:
    enum InterpType {
        INTERP_NEAREST,
        INTERP_LINEAR,
        INTERP_CUBIC
    };

    ImageCutplane();
    virtual ~ImageCutplane();

    virtual const char *getClassName() const
    {
        return "ImageCutplane";
    }

    virtual void setColor(float color[3]);

    virtual void setAmbient(double ambient);

    virtual void setDiffuse(double diffuse);

    virtual void setOpacity(double opacity);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setColorMap(ColorMap *colorMap);

    void setUseWindowLevel(bool state);

    void setWindow(double window);

    void setLevel(double level);

    void setOutlineVisibility(bool state);

    void selectVolumeSlice(Axis axis, double ratio);

    void setSliceVisibility(Axis axis, bool state);

    void setInterpolationType(InterpType type);

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
    Renderer *_renderer;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkImageSlice> _actor[3];
    vtkSmartPointer<vtkActor> _borderActor[3];
    vtkSmartPointer<vtkImageResliceMapper> _mapper[3];
    vtkSmartPointer<vtkPolyDataMapper> _borderMapper[3];
    vtkSmartPointer<vtkPlane> _cutPlane[3];
    vtkSmartPointer<vtkOutlineSource> _outlineSource[3];
};

}

#endif
