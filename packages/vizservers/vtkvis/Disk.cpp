/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkDiskSource.h>
#include <vtkPolyDataNormals.h>
#include <vtkReverseSense.h>

#include "Disk.h"
#include "Trace.h"

using namespace VtkVis;

Disk::Disk() :
    Shape()
{
}

Disk::~Disk()
{
    TRACE("Deleting Disk");
}

void Disk::update()
{
    if (_disk == NULL) {
	_disk = vtkSmartPointer<vtkDiskSource>::New();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalFilter->SetInputConnection(_disk->GetOutputPort());
    normalFilter->AutoOrientNormalsOff();

    _pdMapper->SetInputConnection(normalFilter->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

void Disk::flipNormals(bool state)
{
    if (_disk == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->ReverseCellsOn();
        filter->ReverseNormalsOn();
        filter->SetInputConnection(_disk->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_disk->GetOutputPort());
    }
    _pdMapper->Update();
}
