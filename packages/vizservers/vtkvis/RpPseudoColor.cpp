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
    _dataSet(NULL),
    _edgeWidth(1.0),
    _opacity(1.0),
    _lighting(true)
{
    _edgeColor[0] = 0.0;
    _edgeColor[1] = 0.0;
    _edgeColor[2] = 0.0;
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
 * \brief Specify input DataSet with scalars to colormap
 *
 * Currently the DataSet must be image data (2D uniform grid)
 */
void PseudoColor::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;
        update();
    }
}

/**
 * \brief Returns the DataSet this PseudoColor renders
 */
DataSet *PseudoColor::getDataSet()
{
    return _dataSet;
}

/**
 * \brief Internal method to set up color mapper after a state change
 */
void PseudoColor::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    double dataRange[2];
    _dataSet->getDataRange(dataRange);

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

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        WARN("No scalar point data in dataset %s", _dataSet->getName().c_str());
        if (_lut == NULL) {
            _lut = vtkSmartPointer<vtkLookupTable>::New();
        }
    } else {
        vtkLookupTable *lut = ds->GetPointData()->GetScalars()->GetLookupTable();
        TRACE("Data set scalars lookup table: %p\n", lut);
        if (_lut == NULL) {
            if (lut)
                _lut = lut;
            else
                _lut = vtkSmartPointer<vtkLookupTable>::New();
        }
    }

    _lut->SetRange(dataRange);

    _dsMapper->SetColorModeToMapScalars();
    _dsMapper->UseLookupTableScalarRangeOn();
    _dsMapper->SetLookupTable(_lut);
    //_dsMapper->InterpolateScalarsBeforeMappingOn();

    initProp();
    _dsActor->SetMapper(_dsMapper);
    _dsMapper->Update();
}

/**
 * \brief Get the VTK Prop for the colormapped dataset
 */
vtkProp *PseudoColor::getProp()
{
    return _dsActor;
}

/**
 * \brief Create and initialize a VTK Prop to render the colormapped dataset
 */
void PseudoColor::initProp()
{
    if (_dsActor == NULL) {
        _dsActor = vtkSmartPointer<vtkActor>::New();
        _dsActor->GetProperty()->SetOpacity(_opacity);
        _dsActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _dsActor->GetProperty()->SetLineWidth(_edgeWidth);
        _dsActor->GetProperty()->EdgeVisibilityOff();
        _dsActor->GetProperty()->SetAmbient(.2);
        if (!_lighting)
            _dsActor->GetProperty()->LightingOff();
    }
}

/**
 * \brief Get the VTK colormap lookup table in use
 */
vtkLookupTable *PseudoColor::getLookupTable()
{ 
    return _lut;
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void PseudoColor::setLookupTable(vtkLookupTable *lut)
{
    if (lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
    } else {
        _lut = lut;
    }

    if (_dsMapper != NULL) {
        _dsMapper->UseLookupTableScalarRangeOn();
        _dsMapper->SetLookupTable(_lut);
    }
}

/**
 * \brief Turn on/off rendering of this colormapped dataset
 */
void PseudoColor::setVisibility(bool state)
{
    if (_dsActor != NULL) {
        _dsActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the colormapped dataset
 * 
 * \return Is PseudoColor visible?
 */
bool PseudoColor::getVisibility()
{
    if (_dsActor == NULL) {
        return false;
    } else {
        return (_dsActor->GetVisibility() != 0);
    }
}

/**
 * \brief Set opacity used to render the colormapped dataset
 */
void PseudoColor::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetOpacity(opacity);
}

/**
 * \brief Switch between wireframe and surface representations
 */
void PseudoColor::setWireframe(bool state)
{
    if (_dsActor != NULL) {
        if (state) {
            _dsActor->GetProperty()->SetRepresentationToWireframe();
            _dsActor->GetProperty()->LightingOff();
        } else {
            _dsActor->GetProperty()->SetRepresentationToSurface();
            _dsActor->GetProperty()->SetLighting((_lighting ? 1 : 0));
        }
    }
}

/**
 * \brief Turn on/off rendering of mesh edges
 */
void PseudoColor::setEdgeVisibility(bool state)
{
    if (_dsActor != NULL) {
        _dsActor->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Set RGB color of polygon edges
 */
void PseudoColor::setEdgeColor(float color[3])
{
    _edgeColor[0] = color[0];
    _edgeColor[1] = color[1];
    _edgeColor[2] = color[2];
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
}

/**
 * \brief Set pixel width of polygon edges (may be a no-op)
 */
void PseudoColor::setEdgeWidth(float edgeWidth)
{
    _edgeWidth = edgeWidth;
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetLineWidth(_edgeWidth);
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

/**
 * \brief Turn on/off lighting of this object
 */
void PseudoColor::setLighting(bool state)
{
    _lighting = state;
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetLighting((state ? 1 : 0));
}
