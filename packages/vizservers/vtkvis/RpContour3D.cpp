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

#include "RpContour3D.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Contour3D::Contour3D() :
    VtkGraphicsObject(),
    _numContours(0)
{
    _dataRange[0] = 0;
    _dataRange[1] = 1;
    _color[0] = 0.0f;
    _color[1] = 0.0f;
    _color[2] = 1.0f;
}

Contour3D::~Contour3D()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Contour3D for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Contour3D with NULL DataSet");
#endif
}

/**
 * \brief Specify input DataSet used to extract contours
 */
void Contour3D::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (_dataSet != NULL) {
            double dataRange[2];
            _dataSet->getDataRange(dataRange);
            _dataRange[0] = dataRange[0];
            _dataRange[1] = dataRange[1];
        }

        update();
    }
}

/**
 * \brief Create and initialize a VTK Prop to render isosurfaces
 */
void Contour3D::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkActor>::New();
        vtkProperty *property = getActor()->GetProperty();
        property->EdgeVisibilityOff();
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
 * \brief Get the VTK colormap lookup table in use
 */
vtkLookupTable *Contour3D::getLookupTable()
{ 
    return _lut;
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Contour3D::setLookupTable(vtkLookupTable *lut)
{
    if (lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
    } else {
        _lut = lut;
    }

    if (_contourMapper != NULL) {
        _contourMapper->UseLookupTableScalarRangeOn();
        _contourMapper->SetLookupTable(_lut);
    }
}

/**
 * \brief Internal method to re-compute contours after a state change
 */
void Contour3D::update()
{
    if (_dataSet == NULL) {
        return;
    }
    vtkDataSet *ds = _dataSet->getVtkDataSet();

    initProp();

    if (_dataSet->is2D()) {
        ERROR("DataSet is 2D");
        return;
    }

    // Contour filter to generate isosurfaces
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
            // Generate a 3D unstructured grid
            vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
            mesher->SetInput(pd);
            _contourFilter->SetInputConnection(mesher->GetOutputPort());
        } else {
            // DataSet is a vtkPolyData with lines and/or polygons
            ERROR("Not a 3D DataSet");
            return;
        }
    } else {
         // DataSet is NOT a vtkPolyData
         _contourFilter->SetInput(ds);
    }

    _contourFilter->ComputeNormalsOn();
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

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
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

    if (_lut != NULL) {
        _lut->SetRange(_dataRange);

        _contourMapper->SetColorModeToMapScalars();
        _contourMapper->UseLookupTableScalarRangeOn();
        _contourMapper->SetLookupTable(_lut);
    }

    _contourMapper->Update();
    TRACE("Contour output %d polys, %d strips",
          _contourFilter->GetOutput()->GetNumberOfPolys(),
          _contourFilter->GetOutput()->GetNumberOfStrips());
}

/**
 * \brief Specify number of evenly spaced isosurfaces to render
 *
 * Will override any existing contours
 */
void Contour3D::setContours(int numContours)
{
    _contours.clear();
    _numContours = numContours;

    if (_dataSet != NULL) {
        double dataRange[2];
        _dataSet->getDataRange(dataRange);
        _dataRange[0] = dataRange[0];
        _dataRange[1] = dataRange[1];
    }

    update();
}

/**
 * \brief Specify number of evenly spaced isosurfaces to render
 * between the given range (including range endpoints)
 *
 * Will override any existing contours
 */
void Contour3D::setContours(int numContours, double range[2])
{
    _contours.clear();
    _numContours = numContours;

    _dataRange[0] = range[0];
    _dataRange[1] = range[1];

    update();
}

/**
 * \brief Specify an explicit list of contour isovalues
 *
 * Will override any existing contours
 */
void Contour3D::setContourList(const std::vector<double>& contours)
{
    _contours = contours;
    _numContours = (int)_contours.size();

    update();
}

/**
 * \brief Get the number of contours
 */
int Contour3D::getNumContours() const
{
    return _numContours;
}

/**
 * \brief Get the contour list (may be empty if number of contours
 * was specified in place of a list)
 */
const std::vector<double>& Contour3D::getContourList() const
{
    return _contours;
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Contour3D::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_contourMapper != NULL) {
        _contourMapper->SetClippingPlanes(planes);
    }
}
