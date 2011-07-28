/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>

#include "RpPolyData.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

PolyData::PolyData() :
    _dataSet(NULL),
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
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting PolyData for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting PolyData with NULL DataSet");
#endif
}

/**
 * \brief Get the VTK Prop for the mesh
 */
vtkProp *PolyData::getProp()
{
    return _pdActor;
}

/**
 * \brief Create and initialize a VTK Prop to render a mesh
 */
void PolyData::initProp()
{
    if (_pdActor == NULL) {
        _pdActor = vtkSmartPointer<vtkActor>::New();
        _pdActor->GetProperty()->EdgeVisibilityOn();
        _pdActor->GetProperty()->SetColor(_color[0], _color[1], _color[2]);
        _pdActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _pdActor->GetProperty()->SetLineWidth(_edgeWidth);
        _pdActor->GetProperty()->SetOpacity(_opacity);
        _pdActor->GetProperty()->SetAmbient(.2);
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
        _dataSet = dataSet;
        update();
    }
}

/**
 * \brief Returns the DataSet this PolyData renders
 */
DataSet *PolyData::getDataSet()
{
    return _dataSet;
}

/**
 * \brief Internal method to set up pipeline after a state change
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

    vtkDataSet *ds = _dataSet->getVtkDataSet();
    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        TRACE("Verts: %d Lines: %d Polys: %d Strips: %d",
                  pd->GetNumberOfVerts(),
                  pd->GetNumberOfLines(),
                  pd->GetNumberOfPolys(),
                  pd->GetNumberOfStrips());
        // DataSet is a vtkPolyData
        if (pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            // DataSet is a point cloud
            if (_dataSet->is2D()) {
                vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                mesher->SetInput(pd);
#if defined(DEBUG) && defined(WANT_TRACE)
                mesher->Update();
                vtkPolyData *outpd = mesher->GetOutput();
                TRACE("Delaunay2D Verts: %d Lines: %d Polys: %d Strips: %d",
                      outpd->GetNumberOfVerts(),
                      outpd->GetNumberOfLines(),
                      outpd->GetNumberOfPolys(),
                      outpd->GetNumberOfStrips());
#endif
                _pdMapper->SetInputConnection(mesher->GetOutputPort());
            } else {
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                mesher->SetInput(pd);
                // Delaunay3D returns an UnstructuredGrid, so feed it through a surface filter
                // to get the grid boundary as a PolyData
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->SetInputConnection(mesher->GetOutputPort());
                _pdMapper->SetInputConnection(gf->GetOutputPort());
            }
        } else {
            // DataSet is a vtkPolyData with lines and/or polygons
            _pdMapper->SetInput(pd);
        }
    } else {
        // DataSet is NOT a vtkPolyData
        WARN("DataSet is not a PolyData");
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->SetInput(ds);
        _pdMapper->SetInputConnection(gf->GetOutputPort());
    }

    initProp();
    _pdActor->SetMapper(_pdMapper);
    _pdMapper->Update();
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
