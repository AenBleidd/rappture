/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkStructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkOutlineFilter.h>
#include <vtkStructuredGridOutlineFilter.h>

#include "Outline.h"
#include "Trace.h"

using namespace VtkVis;

Outline::Outline() :
    VtkGraphicsObject()
{
}

Outline::~Outline()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Outline for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Outline with NULL DataSet");
#endif
}

/**
 * \brief Create and initialize a VTK Prop to render a mesh
 */
void Outline::initProp()
{
    VtkGraphicsObject::initProp();
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
void Outline::update()
{
    if (_dataSet == NULL) {
        return;
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    vtkDataSet *ds = _dataSet->getVtkDataSet();
    vtkStructuredGrid *sg = vtkStructuredGrid::SafeDownCast(ds);
    if (sg != NULL) {
        vtkSmartPointer<vtkStructuredGridOutlineFilter> ofilter = vtkSmartPointer<vtkStructuredGridOutlineFilter>::New();
#ifdef USE_VTK6
        ofilter->SetInputData(ds);
#else
        ofilter->SetInput(ds);
#endif
        ofilter->ReleaseDataFlagOn();
        _pdMapper->SetInputConnection(ofilter->GetOutputPort());
    } else {
        vtkSmartPointer<vtkOutlineFilter> ofilter = vtkSmartPointer<vtkOutlineFilter>::New();
#ifdef USE_VTK6
        ofilter->SetInputData(ds);
#else
        ofilter->SetInput(ds);
#endif
        ofilter->ReleaseDataFlagOn();
        _pdMapper->SetInputConnection(ofilter->GetOutputPort());
    }

    initProp();

    setVisibility(_dataSet->getVisibility());

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();

#ifdef WANT_TRACE
    double *b = getBounds();
    TRACE("bounds: %g %g %g %g %g %g", b[0], b[1], b[2], b[3], b[4], b[5]);
#endif
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Outline::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_pdMapper != NULL) {
        _pdMapper->SetClippingPlanes(planes);
    }
}
