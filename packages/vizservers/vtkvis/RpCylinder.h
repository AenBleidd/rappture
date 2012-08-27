/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_CYLINDER_H__
#define __RAPPTURE_VTKVIS_CYLINDER_H__

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCylinderSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK PolyData cylinder
 *
 * This class creates a mesh cylinder
 */
class Cylinder : public Shape
{
public:
    Cylinder();
    virtual ~Cylinder();

    virtual const char *getClassName() const
    {
        return "Cylinder";
    }

    void setCenter(double center[3])
    {
        if (_cylinder != NULL) {
            _cylinder->SetCenter(center);
	}
    }

    void setHeight(double height)
    {
        if (_cylinder != NULL) {
            _cylinder->SetHeight(height);
	}
    }

    void setRadius(double radius)
    {
        if (_cylinder != NULL) {
            _cylinder->SetRadius(radius);
	}
    }

    void setResolution(int res)
    {
        if (_cylinder != NULL) {
            _cylinder->SetResolution(res);
	}
    }

    void setCapping(bool state)
    {
        if (_cylinder != NULL) {
            _cylinder->SetCapping(state ? 1 : 0);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkCylinderSource> _cylinder;
};

}
}

#endif
