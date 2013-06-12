/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkPolyDataNormals.h>
#include <vtkReverseSense.h>

#include "Arrow.h"
#include "Trace.h"

using namespace VtkVis;

Arrow::Arrow() :
    Shape()
{
}

Arrow::~Arrow()
{
    TRACE("Deleting Arrow");
}

void Arrow::update()
{
    if (_arrow == NULL) {
	_arrow = vtkSmartPointer<vtkArrowSource>::New();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalFilter->SetInputConnection(_arrow->GetOutputPort());
    normalFilter->AutoOrientNormalsOff();

    _pdMapper->SetInputConnection(_arrow->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

void Arrow::flipNormals(bool state)
{
    if (_arrow == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->ReverseCellsOn();
        filter->ReverseNormalsOn();
        filter->SetInputConnection(_arrow->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_arrow->GetOutputPort());
    }
}
