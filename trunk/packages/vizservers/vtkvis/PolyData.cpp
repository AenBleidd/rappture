/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkVertexGlyphFilter.h>

#include "PolyData.h"
#include "Trace.h"

using namespace VtkVis;

PolyData::PolyData() :
    GraphicsObject(),
    _cloudStyle(CLOUD_MESH)
{
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
 * \brief Internal method to set up pipeline after a state change
 */
void PolyData::update()
{
    if (_dataSet == NULL) {
        return;
    }

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (_mapper == NULL) {
        _mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _mapper->SetResolveCoincidentTopologyToPolygonOffset();
        // If there are color scalars, use them without lookup table (if scalar visibility is on)
        _mapper->SetColorModeToDefault();
        // Use Point data if available, else cell data
        _mapper->SetScalarModeToDefault();
        _mapper->ScalarVisibilityOff();
    }

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        TRACE("Points: %d Verts: %d Lines: %d Polys: %d Strips: %d",
              pd->GetNumberOfPoints(),
              pd->GetNumberOfVerts(),
              pd->GetNumberOfLines(),
              pd->GetNumberOfPolys(),
              pd->GetNumberOfStrips());
    }
    bool hasNormals = false;
    if ((ds->GetPointData() != NULL &&
         ds->GetPointData()->GetNormals() != NULL) ||
        (ds->GetCellData() != NULL &&
         ds->GetCellData()->GetNormals() != NULL)) {
        hasNormals = true;
    }

    if (_dataSet->isCloud()) {
        // DataSet is a point cloud
        PrincipalPlane plane;
        double offset;
        if (_cloudStyle == CLOUD_POINTS ||
            _dataSet->numDimensions() < 2 || ds->GetNumberOfPoints() < 3) { // 0D or 1D or not enough points to mesh
            vtkSmartPointer<vtkVertexGlyphFilter> vgf = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
            vgf->SetInputData(ds);
#else
            vgf->SetInput(ds);
#endif
            _mapper->SetInputConnection(vgf->GetOutputPort());
        } else if (_dataSet->is2D(&plane, &offset)) {
            vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
            if (plane == PLANE_ZY) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(90, 0, 1, 0);
                if (offset != 0.0) {
                    trans->Translate(-offset, 0, 0);
                }
                mesher->SetTransform(trans);
            } else if (plane == PLANE_XZ) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(-90, 1, 0, 0);
                if (offset != 0.0) {
                    trans->Translate(0, -offset, 0);
                }
                mesher->SetTransform(trans);
            } else if (offset != 0.0) {
                // XY with Z offset
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->Translate(0, 0, -offset);
                mesher->SetTransform(trans);
            }
#ifdef USE_VTK6
            mesher->SetInputData(ds);
#else
            mesher->SetInput(ds);
#endif
            mesher->ReleaseDataFlagOn();
            mesher->Update();
            vtkPolyData *outpd = mesher->GetOutput();
            TRACE("Delaunay2D Verts: %d Lines: %d Polys: %d Strips: %d",
                  outpd->GetNumberOfVerts(),
                  outpd->GetNumberOfLines(),
                  outpd->GetNumberOfPolys(),
                  outpd->GetNumberOfStrips());
            if (outpd->GetNumberOfPolys() == 0) {
                WARN("Delaunay2D mesher failed");
                vtkSmartPointer<vtkVertexGlyphFilter> vgf = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
                vgf->SetInputData(ds);
#else
                vgf->SetInput(ds);
#endif
                _mapper->SetInputConnection(vgf->GetOutputPort());
            } else {
                _mapper->SetInputConnection(mesher->GetOutputPort());
                vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
                normalFilter->SetInputConnection(mesher->GetOutputPort());
                _mapper->SetInputConnection(normalFilter->GetOutputPort());
            }
        } else {
            vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
            mesher->SetInputData(ds);
#else
            mesher->SetInput(ds);
#endif
            mesher->ReleaseDataFlagOn();
            mesher->Update();
            vtkUnstructuredGrid *grid = mesher->GetOutput();
            TRACE("Delaunay3D Cells: %d",
                  grid->GetNumberOfCells());
            if (grid->GetNumberOfCells() == 0) {
                WARN("Delaunay3D mesher failed");
                vtkSmartPointer<vtkVertexGlyphFilter> vgf = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
                vgf->SetInputData(ds);
#else
                vgf->SetInput(ds);
#endif
                _mapper->SetInputConnection(vgf->GetOutputPort());
            } else {
                // Delaunay3D returns an UnstructuredGrid, so feed it 
                // through a surface filter to get the grid boundary 
                // as a PolyData
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->ReleaseDataFlagOn();
                gf->SetInputConnection(mesher->GetOutputPort());
                vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
                normalFilter->SetInputConnection(gf->GetOutputPort());
                _mapper->SetInputConnection(normalFilter->GetOutputPort());
            }
        }
    } else if (pd) {
        // DataSet is a vtkPolyData with cells
        if (hasNormals) {
#ifdef USE_VTK6
            _mapper->SetInputData(pd);
#else
            _mapper->SetInput(pd);
#endif
        } else {
            vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
#ifdef USE_VTK6
            normalFilter->SetInputData(pd);
#else
            normalFilter->SetInput(pd);
#endif
            _mapper->SetInputConnection(normalFilter->GetOutputPort());
        }
    } else {
        // DataSet is NOT a vtkPolyData
        TRACE("DataSet is not a PolyData");
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->UseStripsOn();
        gf->ReleaseDataFlagOn();
#ifdef USE_VTK6
        gf->SetInputData(ds);
#else
        gf->SetInput(ds);
#endif
        vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
        normalFilter->SetInputConnection(gf->GetOutputPort());
        _mapper->SetInputConnection(normalFilter->GetOutputPort());
    }

    initProp();
    getActor()->SetMapper(_mapper);
    _mapper->Update();
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
void PolyData::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_mapper != NULL) {
        _mapper->SetClippingPlanes(planes);
    }
}

void PolyData::setCloudStyle(CloudStyle style)
{
    if (style != _cloudStyle) {
        _cloudStyle = style;
        if (_dataSet != NULL) {
            update();
        }
    }
}
