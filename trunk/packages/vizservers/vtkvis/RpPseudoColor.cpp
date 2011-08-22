/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkDataSetMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkDataSetSurfaceFilter.h>

#include "RpPseudoColor.h"
#include "Trace.h"

#define MESH_POINT_CLOUDS

using namespace Rappture::VtkVis;

PseudoColor::PseudoColor() :
    VtkGraphicsObject(),
    _colorMap(NULL)
{
}

PseudoColor::~PseudoColor()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting PseudoColor for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting PseudoColor with NULL DataSet");
#endif
}

/**
 * \brief Create and initialize a VTK Prop to render the colormapped dataset
 */
void PseudoColor::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkActor>::New();
        vtkProperty *property = getActor()->GetProperty();
        property->SetOpacity(_opacity);
        property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        property->SetLineWidth(_edgeWidth);
        property->EdgeVisibilityOff();
        property->SetAmbient(.2);
        if (!_lighting)
            property->LightingOff();
    }
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
void PseudoColor::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    // Mapper, actor to render color-mapped data set
    if (_dsMapper == NULL) {
        _dsMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    }

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        // DataSet is a vtkPolyData
        if (pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            // DataSet is a point cloud
            if (_dataSet->is2D()) {
#ifdef MESH_POINT_CLOUDS
                vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                mesher->SetInput(pd);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->SetInputConnection(mesher->GetOutputPort());
                _dsMapper->SetInputConnection(gf->GetOutputPort());
#else
                vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                splatter->SetInput(pd);
                int dims[3];
                splatter->GetSampleDimensions(dims);
                TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                dims[2] = 3;
                splatter->SetSampleDimensions(dims);
                double bounds[6];
                splatter->Update();
                splatter->GetModelBounds(bounds);
                TRACE("Model bounds: %g %g %g %g %g %g",
                      bounds[0], bounds[1],
                      bounds[2], bounds[3],
                      bounds[4], bounds[5]);
                vtkSmartPointer<vtkExtractVOI> slicer = vtkSmartPointer<vtkExtractVOI>::New();
                slicer->SetInputConnection(splatter->GetOutputPort());
                slicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                slicer->SetSampleRate(1, 1, 1);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(slicer->GetOutputPort());
                _dsMapper->SetInputConnection(gf->GetOutputPort());
#endif
            } else {
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                mesher->SetInput(pd);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->SetInputConnection(mesher->GetOutputPort());
                _dsMapper->SetInputConnection(gf->GetOutputPort());
             }
        } else {
            // DataSet is a vtkPolyData with lines and/or polygons
            _dsMapper->SetInput(ds);
        }
    } else {
        TRACE("Generating surface for data set");
        // DataSet is NOT a vtkPolyData
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->SetInput(ds);
        _dsMapper->SetInputConnection(gf->GetOutputPort());
    }

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }
    //_dsMapper->InterpolateScalarsBeforeMappingOn();

    initProp();
    getActor()->SetMapper(_dsMapper);
    _dsMapper->Update();
}

void PseudoColor::updateRanges(bool useCumulative,
                               double scalarRange[2],
                               double vectorMagnitudeRange[2],
                               double vectorComponentRange[3][2])
{
    if (useCumulative) {
        _dataRange[0] = scalarRange[0];
        _dataRange[1] = scalarRange[1];
    } else if (_dataSet != NULL) {
        _dataSet->getScalarRange(_dataRange);
    }

    if (_lut != NULL) {
        _lut->SetRange(_dataRange);
    }
}

/**
 * \brief Called when the color map has been edited
 */
void PseudoColor::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void PseudoColor::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (_dsMapper != NULL) {
            _dsMapper->UseLookupTableScalarRangeOn();
            _dsMapper->SetLookupTable(_lut);
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());
    _lut->SetRange(_dataRange);
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void PseudoColor::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_dsMapper != NULL) {
        _dsMapper->SetClippingPlanes(planes);
    }
}
