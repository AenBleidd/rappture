/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_BOX_H__
#define __RAPPTURE_VTKVIS_BOX_H__

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCubeSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK PolyData box
 *
 * This class creates a mesh box
 */
class Box : public Shape
{
public:
    Box();
    virtual ~Box();

    virtual const char *getClassName() const
    {
        return "Box";
    }

    void setSize(double xlen, double ylen, double zlen)
    {
        if (_box != NULL) {
            _box->SetXLength(xlen);
	    _box->SetYLength(ylen);
	    _box->SetZLength(zlen);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkCubeSource> _box;
};

}
}

#endif
