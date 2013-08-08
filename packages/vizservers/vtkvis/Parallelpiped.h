/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_PARALLELPIPED_H
#define VTKVIS_PARALLELPIPED_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkTransform.h>

#include "Shape.h"
#include "DataSet.h"

namespace VtkVis {

/**
 * \brief VTK PolyData Parallelpiped
 *
 * This class creates a mesh parallelpiped
 */
class Parallelpiped : public Shape
{
public:
    Parallelpiped();
    virtual ~Parallelpiped();

    virtual const char *getClassName() const
    {
        return "Parallelpiped";
    }

    void setVectors(double vec1[3], double vec2[3], double vec3[3]);

    void setSize(double xlen, double ylen, double zlen)
    {
        if (_box != NULL) {
            _box->SetXLength(xlen);
	    _box->SetYLength(ylen);
	    _box->SetZLength(zlen);
	}
    }

    void flipNormals(bool state);

private:
    virtual void update();

    vtkSmartPointer<vtkCubeSource> _box;
    vtkSmartPointer<vtkTransform> _transform;
};

}

#endif
