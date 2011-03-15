/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>

#include "RpPseudoColor.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

PseudoColor::PseudoColor() :
    _dataSet(NULL),
    _opacity(1.0),
    _edgeWidth(1.0)
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
    _dataSet = dataSet;
    update();
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
    _dsMapper->SetInput(ds);
    _dsMapper->StaticOff();

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        ERROR("No scalar point data in dataset %s", _dataSet->getName().c_str());
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

    _dsMapper->SetLookupTable(_lut);
    _dsMapper->SetScalarRange(dataRange);
    //_dsMapper->GetLookupTable()->SetRange(dataRange);
    //_dsMapper->InterpolateScalarsBeforeMappingOn();

    initActor();
    _dsActor->SetMapper(_dsMapper);
    //_dsActor->GetProperty()->SetRepresentationToWireframe();
}

/**
 * \brief Get the VTK Actor for the colormapped dataset
 */
vtkActor *PseudoColor::getActor()
{
    return _dsActor;
}

/**
 * \brief Create and initialize a VTK actor to render the colormapped dataset
 */
void PseudoColor::initActor()
{
    if (_dsActor == NULL) {
        _dsActor = vtkSmartPointer<vtkActor>::New();
        _dsActor->GetProperty()->SetOpacity(_opacity);
        _dsActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _dsActor->GetProperty()->SetLineWidth(_edgeWidth);
        _dsActor->GetProperty()->EdgeVisibilityOff();
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

    double dataRange[2];
    if (_dataSet != NULL) {
        _dataSet->getDataRange(dataRange);
        _lut->SetRange(dataRange);
#ifdef notdef
        if (_dataSet->getVtkDataSet()->GetPointData() &&
            _dataSet->getVtkDataSet()->GetPointData()->GetScalars() &&
            _dataSet->getVtkDataSet()->GetPointData()->GetScalars()->GetLookupTable()) {
            TRACE("Change scalar table: %p %p\n", 
                  _dataSet->getVtkDataSet()->GetPointData()->GetScalars()->GetLookupTable(),
                  _lut.GetPointer());
            _dataSet->getVtkDataSet()->GetPointData()->GetScalars()->SetLookupTable(_lut);
            TRACE("Scalar Table: %p\n", _dataSet->getVtkDataSet()->GetPointData()->GetScalars()->GetLookupTable());
        }
#endif
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
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetLighting((state ? 1 : 0));
}
