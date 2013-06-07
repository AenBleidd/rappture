/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCylinderSource.h>
#include <vtkReverseSense.h>

#include "Cylinder.h"
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

void Cylinder::flipNormals(bool state)
{
    if (_cylinder == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->SetInputConnection(_cylinder->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_cylinder->GetOutputPort());
    }
}
