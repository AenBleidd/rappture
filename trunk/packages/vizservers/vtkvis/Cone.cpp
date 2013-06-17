/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkPolyDataNormals.h>
#include <vtkReverseSense.h>

#include "Cone.h"
#include "Trace.h"

using namespace VtkVis;

Cone::Cone() :
    Shape()
{
}

Cone::~Cone()
{
    TRACE("Deleting Cone");
}

void Cone::update()
{
    if (_cone == NULL) {
	_cone = vtkSmartPointer<vtkConeSource>::New();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalFilter->SetFeatureAngle(90.);
    normalFilter->SetInputConnection(_cone->GetOutputPort());

    _pdMapper->SetInputConnection(_cone->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

void Cone::flipNormals(bool state)
{
    if (_cone == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->ReverseCellsOn();
        filter->ReverseNormalsOn();
        filter->SetInputConnection(_cone->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_cone->GetOutputPort());
    }
}
