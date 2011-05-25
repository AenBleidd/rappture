/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_PSEUDOCOLOR_H__
#define __RAPPTURE_VTKVIS_PSEUDOCOLOR_H__

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>

#include <vector>

#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Color-mapped plot of data set
 */
class PseudoColor {
public:
    PseudoColor();
    virtual ~PseudoColor();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getActor();

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setVisibility(bool state);

    bool getVisibility();

    void setOpacity(double opacity);

    void setEdgeVisibility(bool state);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    void initActor();
    void update();

    DataSet * _dataSet;

    double _opacity;
    float _edgeColor[3];
    float _edgeWidth;
    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
    vtkSmartPointer<vtkActor> _dsActor;
};

}
}

#endif
