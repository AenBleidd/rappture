/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRegularPolygonSource.h>

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

    _pdMapper->SetInputConnection(_polygon->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}
