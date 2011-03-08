/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include "RpPolyData.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

PolyData::PolyData() :
    _edgeWidth(1.0),
    _opacity(1.0),
    _lighting(true)
{
    _color[0] = 0.0;
    _color[1] = 0.0;
    _color[2] = 1.0;
    _edgeColor[0] = 0.0;
    _edgeColor[1] = 0.0;
    _edgeColor[2] = 0.0;
}

PolyData::~PolyData()
{
}

/**
 * \brief Get the VTK Actor for the mesh
 */
vtkActor *PolyData::getActor()
{
    return _pdActor;
}

/**
 * \brief Create and initialize a VTK actor to render a mesh
 */
void PolyData::initActor()
{
    if (_pdActor == NULL) {
        _pdActor = vtkSmartPointer<vtkActor>::New();
        _pdActor->GetProperty()->EdgeVisibilityOn();
        _pdActor->GetProperty()->SetColor(_color[0], _color[1], _color[2]);
        _pdActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _pdActor->GetProperty()->SetLineWidth(_edgeWidth);
        _pdActor->GetProperty()->SetOpacity(_opacity);
        if (!_lighting)
            _pdActor->GetProperty()->LightingOff();
    }
}

/**
 * \brief Specify input DataSet (PolyData)
 *
 * The DataSet must be a PolyData object
 */
void PolyData::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        vtkDataSet *ds = dataSet->getVtkDataSet();
        if (!ds->IsA("vtkPolyData")) {
            ERROR("DataSet is not a PolyData");
            return;
        }
        TRACE("DataSet is a vtkPolyData");
        _dataSet = dataSet;
        update();
    }
}

/**
 * \brief Internal method to re-compute contours after a state change
 */
void PolyData::update()
{
    if (_dataSet == NULL) {
        return;
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    initActor();
    _pdActor->SetMapper(_pdMapper);

    _pdMapper->SetInput(vtkPolyData::SafeDownCast(_dataSet->getVtkDataSet()));
}

/**
 * \brief Turn on/off rendering of this mesh
 */
void PolyData::setVisibility(bool state)
{
    if (_pdActor != NULL) {
        _pdActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the mesh
 * 
 * \return Is mesh visible?
 */
bool PolyData::getVisibility()
{
    if (_pdActor == NULL) {
        return false;
    } else {
        return (_pdActor->GetVisibility() != 0);
    }
}

/**
 * \brief Set opacity used to render the mesh
 */
void PolyData::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_pdActor != NULL)
        _pdActor->GetProperty()->SetOpacity(opacity);
}

/**
 * \brief Switch between wireframe and surface representations
 */
void PolyData::setWireframe(bool state)
{
    if (_pdActor != NULL) {
        if (state) {
            _pdActor->GetProperty()->SetRepresentationToWireframe();
            _pdActor->GetProperty()->LightingOff();
        } else {
            _pdActor->GetProperty()->SetRepresentationToSurface();
            _pdActor->GetProperty()->SetLighting((_lighting ? 1 : 0));
        }
    }
}

/**
 * \brief Set RGB color of polygon faces
 */
void PolyData::setColor(float color[3])
{
    _color[0] = color[0];
    _color[1] = color[1];
    _color[2] = color[2];
    if (_pdActor != NULL)
        _pdActor->GetProperty()->SetColor(_color[0], _color[1], _color[2]);
}

/**
 * \brief Turn on/off rendering of mesh edges
 */
void PolyData::setEdgeVisibility(bool state)
{
    if (_pdActor != NULL) {
        _pdActor->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Set RGB color of polygon edges
 */
void PolyData::setEdgeColor(float color[3])
{
    _edgeColor[0] = color[0];
    _edgeColor[1] = color[1];
    _edgeColor[2] = color[2];
    if (_pdActor != NULL)
        _pdActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
}

/**
 * \brief Set pixel width of polygon edges (may be a no-op)
 */
void PolyData::setEdgeWidth(float edgeWidth)
{
    _edgeWidth = edgeWidth;
    if (_pdActor != NULL)
        _pdActor->GetProperty()->SetLineWidth(_edgeWidth);
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void PolyData::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_pdMapper != NULL) {
        _pdMapper->SetClippingPlanes(planes);
    }
}

/**
 * \brief Turn on/off lighting of this object
 */
void PolyData::setLighting(bool state)
{
    _lighting = state;
    if (_pdActor != NULL)
        _pdActor->GetProperty()->SetLighting((state ? 1 : 0));
}
