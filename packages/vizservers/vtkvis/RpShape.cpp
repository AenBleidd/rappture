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

#include "RpShape.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Shape::Shape() :
    VtkGraphicsObject()
{
}

Shape::~Shape()
{
    //TRACE("Deleting Shape");
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Shape::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_pdMapper != NULL) {
        _pdMapper->SetClippingPlanes(planes);
    }
}
