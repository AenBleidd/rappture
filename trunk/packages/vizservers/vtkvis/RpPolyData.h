/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_POLYDATA_H__
#define __RAPPTURE_VTKVIS_POLYDATA_H__

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
class PolyData : public VtkGraphicsObject {
public:
    PolyData();
    virtual ~PolyData();

    virtual const char *getClassName() const
    {
        return "PolyData";
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

private:
    virtual void initProp();
    virtual void update();

    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
};

}
}

#endif
