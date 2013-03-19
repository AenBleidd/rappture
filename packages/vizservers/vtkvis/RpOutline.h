/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_OUTLINE_H__
#define __RAPPTURE_VTKVIS_OUTLINE_H__

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

#include "RpVtkGraphicsObject.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK Wire outline of DataSet
 *
 * This class creates a wireframe box around the DataSet bounds
 */
class Outline : public VtkGraphicsObject {
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
}

#endif
