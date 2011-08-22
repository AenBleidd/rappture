/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>

#include "RpContour2D.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Contour2D::Contour2D(int numContours) :
    VtkGraphicsObject(),
    _numContours(numContours)
{
}

Contour2D::Contour2D(const std::vector<double>& contours) :
    VtkGraphicsObject(),
    _numContours(contours.size()),
    _contours(contours)
{
}

Contour2D::~Contour2D()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Contour2D for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Contour2D with NULL DataSet");
#endif
}

/**
 * \brief Create and initialize a VTK Prop to render isolines
 */
void Contour2D::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkActor>::New();
        vtkProperty *property = getActor()->GetProperty();
        property->EdgeVisibilityOn();
        property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        property->SetLineWidth(_edgeWidth);
        property->SetOpacity(_opacity);
        property->SetAmbient(.2);
        property->LightingOff();
    }
}

/**
 * \brief Internal method to re-compute contours after a state change
 */
void Contour2D::update()
{
    if (_dataSet == NULL) {
        return;
    }
    vtkDataSet *ds = _dataSet->getVtkDataSet();

    initProp();

    // Contour filter to generate isolines
    if (_contourFilter == NULL) {
        _contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    }

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        WARN("No scalar point data in dataset %s", _dataSet->getName().c_str());
        if (ds->GetCellData() != NULL &&
            ds->GetCellData()->GetScalars() != NULL) {
            cellToPtData = 
                vtkSmartPointer<vtkCellDataToPointData>::New();
            cellToPtData->SetInput(ds);
            //cellToPtData->PassCellDataOn();
            cellToPtData->Update();
            ds = cellToPtData->GetOutput();
        } else {
            ERROR("No scalar cell data in dataset %s", _dataSet->getName().c_str());
        }
    }

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        // DataSet is a vtkPolyData
        if (pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            // DataSet is a point cloud
            if (_dataSet->is2D()) {
                vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                mesher->SetInput(pd);
                _contourFilter->SetInputConnection(mesher->GetOutputPort());
            } else {
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                mesher->SetInput(pd);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->SetInputConnection(mesher->GetOutputPort());
                _contourFilter->SetInputConnection(gf->GetOutputPort());
            }
        } else {
            // DataSet is a vtkPolyData with lines and/or polygons
            _contourFilter->SetInput(ds);
        }
    } else {
        TRACE("Generating surface for data set");
        // DataSet is NOT a vtkPolyData
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->SetInput(ds);
        _contourFilter->SetInputConnection(gf->GetOutputPort());
    }

    _contourFilter->ComputeNormalsOff();
    _contourFilter->ComputeGradientsOff();

    // Speed up multiple contour computation at cost of extra memory use
    if (_numContours > 1) {
        _contourFilter->UseScalarTreeOn();
    } else {
        _contourFilter->UseScalarTreeOff();
    }

    _contourFilter->SetNumberOfContours(_numContours);

    if (_contours.empty()) {
        // Evenly spaced isovalues
        _contourFilter->GenerateValues(_numContours, _dataRange[0], _dataRange[1]);
    } else {
        // User-supplied isovalues
        for (int i = 0; i < _numContours; i++) {
            _contourFilter->SetValue(i, _contours[i]);
        }
    }
    if (_contourMapper == NULL) {
        _contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _contourMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _contourMapper->SetInputConnection(_contourFilter->GetOutputPort());
        getActor()->SetMapper(_contourMapper);
    }

    _contourMapper->Update();
}

void Contour2D::updateRanges(bool useCumulative,
                             double scalarRange[2],
                             double vectorMagnitudeRange[2],
                             double vectorComponentRange[3][2])
{
    if (useCumulative) {
        _dataRange[0] = scalarRange[0];
        _dataRange[1] = scalarRange[1];
    } else if (_dataSet != NULL) {
        _dataSet->getScalarRange(_dataRange);
    } else {
        WARN("updateRanges called before setDataSet");
    }

    if (_contours.empty() && _numContours > 0) {
        // Contour isovalues need to be recomputed
        update();
    }
}

/**
 * \brief Specify number of evenly spaced contour lines to render
 *
 * Will override any existing contours
 */
void Contour2D::setContours(int numContours)
{
    _contours.clear();
    _numContours = numContours;

    update();
}

/**
 * \brief Specify an explicit list of contour isovalues
 *
 * Will override any existing contours
 */
void Contour2D::setContourList(const std::vector<double>& contours)
{
    _contours = contours;
    _numContours = (int)_contours.size();

    update();
}

/**
 * \brief Get the number of contours
 */
int Contour2D::getNumContours() const
{
    return _numContours;
}

/**
 * \brief Get the contour list (may be empty if number of contours
 * was specified in place of a list)
 */
const std::vector<double>& Contour2D::getContourList() const
{
    return _contours;
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Contour2D::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_contourMapper != NULL) {
        _contourMapper->SetClippingPlanes(planes);
    }
}
