/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCylinderSource.h>

#include "RpCylinder.h"
#include "Trace.h"

using namespace VtkVis;

Cylinder::Cylinder() :
    Shape()
{
}

Cylinder::~Cylinder()
{
    TRACE("Deleting Cylinder");
}

void Cylinder::update()
{
    if (_cylinder == NULL) {
	_cylinder = vtkSmartPointer<vtkCylinderSource>::New();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    _pdMapper->SetInputConnection(_cylinder->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}
