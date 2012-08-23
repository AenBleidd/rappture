/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_SPHERE_H__
#define __RAPPTURE_VTKVIS_SPHERE_H__

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSphereSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK PolyData sphere
 *
 * This class creates a mesh sphere
 */
class Sphere : public Shape
{
public:
    Sphere();
    virtual ~Sphere();

    virtual const char *getClassName() const
    {
        return "Sphere";
    }

    void setThetaResolution(int res)
    {
        if (_sphere != NULL)
            _sphere->SetThetaResolution(res);
    }

    void setPhiResolution(int res)
    {
        if (_sphere != NULL)
            _sphere->SetPhiResolution(res);
    }

    void setStartTheta(double val)
    {
        if (_sphere != NULL)
            _sphere->SetStartTheta(val);
    }

    void setEndTheta(double val)
    {
        if (_sphere != NULL)
            _sphere->SetEndTheta(val);
    }

    void setStartPhi(double val)
    {
        if (_sphere != NULL)
            _sphere->SetStartPhi(val);
    }

    void setEndPhi(double val)
    {
        if (_sphere != NULL)
            _sphere->SetEndPhi(val);
    }

    void setLatLongTessellation(bool val)
    {
        if (_sphere != NULL)
            _sphere->SetLatLongTessellation((val ? 1 : 0));
    }

    void setRadius(double radius)
    {
        if (_sphere != NULL)
            _sphere->SetRadius(radius);
    }

private:
    virtual void update();

    vtkSmartPointer<vtkSphereSource> _sphere;
};

}
}

#endif
