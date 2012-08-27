/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkArrowSource.h>

#include "RpArrow.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

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

    _pdMapper->SetInputConnection(_arrow->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}
