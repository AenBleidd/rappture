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

    vtkActor *getActor();

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setVisibility(bool state);

    void setOpacity(double opacity);

    void setClippingPlanes(vtkPlaneCollection *planes);

private:
    void initActor();
    void update();

    DataSet * _dataSet;

    double _opacity;
    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
    vtkSmartPointer<vtkActor> _dsActor;
};

}
}

#endif
