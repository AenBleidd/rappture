/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_POLYDATA_H__
#define __RAPPTURE_VTKVIS_POLYDATA_H__

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

#include <vector>

#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK Mesh (Polygon data)
 */
class PolyData {
public:
    PolyData();
    virtual ~PolyData();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void setVisibility(bool state);

    bool getVisibility();

    void setOpacity(double opacity);

    void setWireframe(bool state);

    void setColor(float color[3]);

    void setEdgeVisibility(bool state);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    void initProp();
    void update();

    DataSet *_dataSet;

    float _color[3];
    float _edgeColor[3];
    float _edgeWidth;
    double _opacity;
    bool _lighting;
    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
    vtkSmartPointer<vtkActor> _pdActor;
};

}
}

#endif
