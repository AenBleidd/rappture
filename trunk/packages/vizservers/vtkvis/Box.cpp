/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkCubeSource.h>
#include <vtkReverseSense.h>

#include "Box.h"
#include "Trace.h"

using namespace VtkVis;

Box::Box() :
    Shape()
{
}

Box::~Box()
{
    TRACE("Deleting Box");
}

void Box::update()
{
    if (_box == NULL) {
	_box = vtkSmartPointer<vtkCubeSource>::New();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    _pdMapper->SetInputConnection(_box->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

void Box::flipNormals(bool state)
{
    if (_box == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->ReverseCellsOn();
        filter->ReverseNormalsOn();
        filter->SetInputConnection(_box->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_box->GetOutputPort());
    }
}
