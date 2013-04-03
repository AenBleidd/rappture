/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_POLYDATA_H
#define VTKVIS_POLYDATA_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

#include "VtkGraphicsObject.h"
#include "VtkDataSet.h"

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

#endif
