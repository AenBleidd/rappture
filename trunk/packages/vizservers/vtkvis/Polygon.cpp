/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRegularPolygonSource.h>
#include <vtkPolyDataNormals.h>
#include <vtkReverseSense.h>

#include "Polygon.h"
#include "Trace.h"

using namespace VtkVis;

Polygon::Polygon() :
    Shape()
{
}

Polygon::~Polygon()
{
    TRACE("Deleting Polygon");
}

void Polygon::update()
{
    if (_polygon == NULL) {
	_polygon = vtkSmartPointer<vtkRegularPolygonSource>::New();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalFilter->SetInputConnection(_polygon->GetOutputPort());

    _pdMapper->SetInputConnection(_polygon->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

void Polygon::flipNormals(bool state)
{
    if (_polygon == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->ReverseCellsOn();
        filter->ReverseNormalsOn();
        filter->SetInputConnection(_polygon->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_polygon->GetOutputPort());
    }
    _pdMapper->Update();
}

