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

#include "RpVtkGraphicsObject.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Color-mapped plot of data set
 * 
 * Currently the DataSet must be image data (2D uniform grid)
 */
class PseudoColor : public VtkGraphicsObject {
public:
    PseudoColor();
    virtual ~PseudoColor();

    virtual const char *getClassName() const
    {
        return "PseudoColor";
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

private:
    virtual void initProp();
    virtual void update();

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
};

}
}

#endif
