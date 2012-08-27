/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkConeSource.h>

#include "RpCone.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

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

    _pdMapper->SetInputConnection(_cone->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}
