/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkCharArray.h>
#include <vtkDataSetReader.h>
#include <vtkDataSetMapper.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>

#include "RpVtkDataSet.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

DataSet::DataSet(const std::string& name) :
    _name(name),
    _visible(true)
{
    _dataRange[0] = 0;
    _dataRange[1] = 1;
    for (int i = 0; i < 6; i++) {
        _bounds[i] = 0;
    }
}

DataSet::~DataSet()
{
}

/**
 * \brief Set visibility flag in DataSet
 *
 * This method is used for record-keeping.  The renderer controls
 * the visibility of related graphics objects.
 */
void DataSet::setVisibility(bool state)
{
    _visible = state;
}

/**
 * \brief Get visibility flag in DataSet
 *
 * This method is used for record-keeping.  The renderer controls
 * the visibility of related graphics objects.
 */
bool DataSet::getVisibility() const
{
    return _visible;
}

/**
 * \brief Read a VTK data file
 */
bool DataSet::setDataFile(const char *filename)
{
    vtkSmartPointer<vtkDataSetReader> reader = vtkSmartPointer<vtkDataSetReader>::New();
    reader->SetFileName(filename);
    return setData(reader);
}

/**
 * \brief Read a VTK data file from a memory buffer
 */
bool DataSet::setData(char *data, int nbytes)
{
    vtkSmartPointer<vtkDataSetReader> reader = vtkSmartPointer<vtkDataSetReader>::New();
    vtkSmartPointer<vtkCharArray> dataSetString = vtkSmartPointer<vtkCharArray>::New();

    dataSetString->SetArray(data, nbytes, 1);
    reader->SetInputArray(dataSetString);
    reader->ReadFromInputStringOn();
    return setData(reader);
}

/**
 * \brief Read dataset using supplied reader
 */
bool DataSet::setData(vtkDataSetReader *reader)
{
    // Force reading data set
    reader->SetLookupTableName("");
    _dataSet = reader->GetOutput();
    _dataSet->Update();
    _dataSet->GetScalarRange(_dataRange);

    TRACE("DataSet class: %s", _dataSet->GetClassName());
    TRACE("Scalar Range: %.12e, %.12e", _dataRange[0], _dataRange[1]);
    return true;
}

/**
 * \brief Set dataset from existing vtkDataSet object
 */
bool DataSet::setData(vtkDataSet *ds)
{
    _dataSet = ds;
    _dataSet->Update();
    _dataSet->GetScalarRange(_dataRange);
    _dataSet->GetBounds(_bounds);

    TRACE("DataSet class: %s", _dataSet->GetClassName());
    TRACE("Scalar Range: %.12e, %.12e", _dataRange[0], _dataRange[1]);
    return true;
}

/**
 * \brief Does DataSet lie in the XY plane (z = 0)
 */
bool DataSet::is2D() const
{
    return (_bounds[4] == 0. && _bounds[4] == _bounds[5]);
}

/**
 * \brief Get the name/id of this dataset
 */
const std::string& DataSet::getName() const
{
    return _name;
}

/**
 * \brief Get the underlying VTK DataSet object
 */
vtkDataSet *DataSet::getVtkDataSet()
{
    return _dataSet;
}

/**
 * \brief Get the underlying VTK DataSet subclass class name
 */
const char *DataSet::getVtkType()
{
    return _dataSet->GetClassName();
}

/**
 * \brief Get the range of scalar values in the DataSet
 */
void DataSet::getDataRange(double minmax[2])
{
    minmax[0] = _dataRange[0];
    minmax[1] = _dataRange[1];
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 *
 * \return the value of the nearest point or 0 if no scalar data available
 */
double DataSet::getDataValue(double x, double y, double z)
{
    if (_dataSet == NULL)
        return 0;
    if (_dataSet->GetPointData() == NULL ||
        _dataSet->GetPointData()->GetScalars() == NULL) {
        return 0.0;
    }
    vtkIdType pt = _dataSet->FindPoint(x, y, z);
    return _dataSet->GetPointData()->GetScalars()->GetComponent(pt, 0);
}
