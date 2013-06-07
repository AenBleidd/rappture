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

#include "Disk.h"
#include "Trace.h"

using namespace VtkVis;

Disk::Disk() :
    Shape(),
    _flipNormals(false)
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
    normalFilter->SetFlipNormals(_flipNormals ? 1 : 0);

    _pdMapper->SetInputConnection(normalFilter->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

void Disk::flipNormals(bool state)
{
    if (_flipNormals != state) {
        _flipNormals = state;
        update();
    }
}
