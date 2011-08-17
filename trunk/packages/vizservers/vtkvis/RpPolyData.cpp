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
    VtkGraphicsObject()
{
    _color[0] = 0.0f;
    _color[1] = 0.0f;
    _color[2] = 1.0f;
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
 * \brief Create and initialize a VTK Prop to render a mesh
 */
void PolyData::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkActor>::New();
        vtkProperty *property = getActor()->GetProperty();
        property->EdgeVisibilityOn();
        property->SetColor(_color[0], _color[1], _color[2]);
        property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        property->SetLineWidth(_edgeWidth);
        property->SetOpacity(_opacity);
        property->SetAmbient(.2);
        if (!_lighting)
            property->LightingOff();
    }
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
                mesher->ReleaseDataFlagOn();
                _pdMapper->SetInputConnection(mesher->GetOutputPort());
#if defined(DEBUG) && defined(WANT_TRACE)
                mesher->Update();
                vtkPolyData *outpd = mesher->GetOutput();
                TRACE("Delaunay2D Verts: %d Lines: %d Polys: %d Strips: %d",
                      outpd->GetNumberOfVerts(),
                      outpd->GetNumberOfLines(),
                      outpd->GetNumberOfPolys(),
                      outpd->GetNumberOfStrips());
#endif
            } else {
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                mesher->SetInput(pd);
                mesher->ReleaseDataFlagOn();
                // Delaunay3D returns an UnstructuredGrid, so feed it through a surface filter
                // to get the grid boundary as a PolyData
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->SetInputConnection(mesher->GetOutputPort());
                gf->ReleaseDataFlagOn();
                _pdMapper->SetInputConnection(gf->GetOutputPort());
            }
        } else {
            // DataSet is a vtkPolyData with lines and/or polygons
            _pdMapper->SetInput(pd);
        }
    } else {
        // DataSet is NOT a vtkPolyData
        TRACE("DataSet is not a PolyData");
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->SetInput(ds);
        gf->ReleaseDataFlagOn();
        _pdMapper->SetInputConnection(gf->GetOutputPort());
    }

    initProp();
    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
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

