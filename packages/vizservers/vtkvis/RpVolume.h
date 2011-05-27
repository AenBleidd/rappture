/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_VOLUME_H__
#define __RAPPTURE_VTKVIS_VOLUME_H__

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkVolume.h>
#include <vtkAbstractVolumeMapper.h>
#include <vtkPlaneCollection.h>

#include "RpVtkDataSet.h"
#include "ColorMap.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Volume Rendering
 */
class Volume {
public:
    enum BlendMode {
        COMPOSITE = 0,
        MAX_INTENSITY,
        MIN_INTENSITY
    };

    Volume();
    virtual ~Volume();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void setColorMap(ColorMap *cmap);
    void setColorMap(ColorMap *cmap, double dataRange[2]);

    ColorMap *getColorMap();

    void setOpacity(double opacity);

    void setVisibility(bool state);

    bool getVisibility();

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setAmbient(double coeff);

    void setDiffuse(double coeff);

    void setSpecular(double coeff, double power);

    void setLighting(bool state);

private:
    void initProp();
    void update();

    DataSet *_dataSet;
    double _opacity;

    ColorMap *_colorMap;
    vtkSmartPointer<vtkVolume> _volumeProp;
    vtkSmartPointer<vtkAbstractVolumeMapper> _volumeMapper;
};

}
}

#endif
