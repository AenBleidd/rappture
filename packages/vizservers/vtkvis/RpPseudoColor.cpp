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
    _opacity(1.0)
{
}

PseudoColor::~PseudoColor()
{
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

    vtkLookupTable *lut = ds->GetPointData()->GetScalars()->GetLookupTable();
    TRACE("Data set scalars lookup table: %p\n", lut);
    if (_lut == NULL) {
        if (lut)
            _lut = lut;
        else
            _lut = vtkSmartPointer<vtkLookupTable>::New();
    } else {
        
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
 * \brief Get the VTK Actor for the colormap iamge
 */
vtkActor *PseudoColor::getActor()
{
    return _dsActor;
}

/**
 * \brief Create and initialize a VTK actor to render the colormap image
 */
void PseudoColor::initActor()
{
    if (_dsActor == NULL) {
        _dsActor = vtkSmartPointer<vtkActor>::New();
        _dsActor->GetProperty()->SetOpacity(_opacity);
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

    if (_dataSet != NULL) {
        double dataRange[2];
        _dataSet->getDataRange(dataRange);
        _lut->SetRange(dataRange);

        if (_dataSet->getVtkDataSet()->GetPointData()->GetScalars()->GetLookupTable()) {
            TRACE("Change scalar table: %p %p\n", 
                  _dataSet->getVtkDataSet()->GetPointData()->GetScalars()->GetLookupTable(),
                  _lut.GetPointer());
            _dataSet->getVtkDataSet()->GetPointData()->GetScalars()->SetLookupTable(_lut);
            TRACE("Scalar Table: %p\n", _dataSet->getVtkDataSet()->GetPointData()->GetScalars()->GetLookupTable());
        }
    }
    if (_dsMapper != NULL) {
        _dsMapper->SetLookupTable(_lut);
    }
}

/**
 * \brief Turn on/off rendering of this colormap image
 */
void PseudoColor::setVisibility(bool state)
{
    if (_dsActor != NULL) {
        _dsActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Set opacity used to render the colormap image
 */
void PseudoColor::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetOpacity(opacity);
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 */
void PseudoColor::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_dsMapper != NULL)
        _dsMapper->SetClippingPlanes(planes);
}
