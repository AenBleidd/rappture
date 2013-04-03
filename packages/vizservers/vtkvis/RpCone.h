/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_CONE_H
#define VTKVIS_CONE_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkConeSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace VtkVis {

/**
 * \brief VTK PolyData cone
 *
 * This class creates a mesh cone
 */
class Cone : public Shape
{
public:
    Cone();
    virtual ~Cone();

    virtual const char *getClassName() const
    {
        return "Cone";
    }

    void setCenter(double center[3])
    {
        if (_cone != NULL) {
            _cone->SetCenter(center);
	}
    }

    void setHeight(double height)
    {
        if (_cone != NULL) {
            _cone->SetHeight(height);
	}
    }

    void setRadius(double radius)
    {
        if (_cone != NULL) {
            _cone->SetRadius(radius);
	}
    }

    void setDirection(double x, double y, double z)
    {
        if (_cone != NULL) {
            _cone->SetDirection(x, y, z);
	}
    }

    void setResolution(int res)
    {
        if (_cone != NULL) {
            _cone->SetResolution(res);
	}
    }

    void setCapping(bool state)
    {
        if (_cone != NULL) {
            _cone->SetCapping(state ? 1 : 0);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkConeSource> _cone;
};

}

#endif
