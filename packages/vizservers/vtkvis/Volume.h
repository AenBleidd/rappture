/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_VOLUME_H
#define VTKVIS_VOLUME_H

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkVolume.h>
#include <vtkAbstractVolumeMapper.h>
#include <vtkPlaneCollection.h>

#include "GraphicsObject.h"
#include "ColorMap.h"

namespace VtkVis {

/**
 * \brief Volume Rendering
 *
 * Currently the DataSet must be image data (3D uniform grid),
 * or an UnstructuredGrid
 */
class Volume : public GraphicsObject {
public:
    enum BlendMode {
        COMPOSITE = 0,
        MAX_INTENSITY,
        MIN_INTENSITY
    };

    Volume();
    virtual ~Volume();

    virtual const char *getClassName() const
    {
        return "Volume";
    }

    void getSpacing(double spacing[3]);

    double getAverageSpacing();

    virtual void setOpacity(double opacity);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setColorMap(ColorMap *cmap);

    ColorMap *getColorMap();

    void updateColorMap();

    virtual void updateRanges(Renderer *renderer);

    void setSampleDistance(float d);

private:
    virtual void initProp();
    virtual void update();

    ColorMap *_colorMap;
    vtkSmartPointer<vtkAbstractVolumeMapper> _volumeMapper;
};

}

#endif
