/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkDiskSource.h>

#include "RpDisk.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

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

    _pdMapper->SetInputConnection(_disk->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}
