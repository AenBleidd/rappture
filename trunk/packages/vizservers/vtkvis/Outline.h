/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_OUTLINE_H
#define VTKVIS_OUTLINE_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

#include "GraphicsObject.h"
#include "DataSet.h"

namespace VtkVis {

/**
 * \brief VTK Wire outline of DataSet
 *
 * This class creates a wireframe box around the DataSet bounds
 */
class Outline : public GraphicsObject {
public:
    Outline();
    virtual ~Outline();

    virtual const char *getClassName() const
    {
        return "Outline";
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

private:
    virtual void initProp();
    virtual void update();

    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
};

}

#endif
