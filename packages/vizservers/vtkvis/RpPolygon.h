/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_POLYGON_H
#define VTKVIS_POLYGON_H_

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRegularPolygonSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace VtkVis {

/**
 * \brief VTK PolyData regular n-sided polygon
 *
 * This class creates a regular n-sided polygon
 */
class Polygon : public Shape
{
public:
    Polygon();
    virtual ~Polygon();

    virtual const char *getClassName() const
    {
        return "Polygon";
    }

    void setNumberOfSides(int numSides)
    {
        if (_polygon != NULL) {
            _polygon->SetNumberOfSides(numSides);
	}
    }

    void setCenter(double center[3])
    {
        if (_polygon != NULL) {
            _polygon->SetCenter(center);
	}
    }

    void setNormal(double normal[3])
    {
        if (_polygon != NULL) {
            _polygon->SetNormal(normal);
	}
    }

    void setRadius(double radius)
    {
        if (_polygon != NULL) {
            _polygon->SetRadius(radius);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkRegularPolygonSource> _polygon;
};

}

#endif
