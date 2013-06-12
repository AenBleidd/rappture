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
#include <vtkSphereSource.h>
#include <vtkReverseSense.h>

#include "Sphere.h"
#include "Trace.h"

using namespace VtkVis;

Sphere::Sphere() :
    Shape()
{
}

Sphere::~Sphere()
{
    TRACE("Deleting Sphere");
}

void Sphere::update()
{
    if (_sphere == NULL) {
	_sphere = vtkSmartPointer<vtkSphereSource>::New();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    _pdMapper->SetInputConnection(_sphere->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

void Sphere::flipNormals(bool state)
{
    if (_sphere == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->ReverseCellsOn();
        filter->ReverseNormalsOn();
        filter->SetInputConnection(_sphere->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_sphere->GetOutputPort());
    }
}
