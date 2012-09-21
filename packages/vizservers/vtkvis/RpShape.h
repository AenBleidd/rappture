/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_SHAPE_H__
#define __RAPPTURE_VTKVIS_SHAPE_H__

#include <cassert>

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

#include "RpVtkGraphicsObject.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK Mesh (Polygon data)
 *
 * This class creates a boundary mesh of a DataSet
 */
class Shape : public VtkGraphicsObject {
public:
    Shape();
    virtual ~Shape();

    virtual const char *getClassName() const
    {
        return "Shape";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer)
    {
        assert(dataSet == NULL);
        update();
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

protected:
    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
};

}
}

#endif
