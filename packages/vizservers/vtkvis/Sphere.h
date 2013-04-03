/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_SPHERE_H
#define VTKVIS_SPHERE_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSphereSource.h>

#include "Shape.h"
#include "VtkDataSet.h"

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

#endif
