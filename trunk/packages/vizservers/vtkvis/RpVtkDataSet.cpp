/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkCharArray.h>
#include <vtkDataSetReader.h>
#include <vtkPolyData.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkUnstructuredGrid.h>
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

#if defined(WANT_TRACE) && defined(DEBUG)
    reader->DebugOn();
#endif

    reader->SetFileName(filename);
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    return setData(reader);
}

/**
 * \brief Read a VTK data file from a memory buffer
 */
bool DataSet::setData(char *data, int nbytes)
{
    TRACE("Entering");

    vtkSmartPointer<vtkDataSetReader> reader = vtkSmartPointer<vtkDataSetReader>::New();
    vtkSmartPointer<vtkCharArray> dataSetString = vtkSmartPointer<vtkCharArray>::New();

#if defined(WANT_TRACE) && defined(DEBUG)
    reader->DebugOn();
    dataSetString->DebugOn();
#endif

    dataSetString->SetArray(data, nbytes, 1);
    reader->SetInputArray(dataSetString);
    reader->ReadFromInputStringOn();
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();

    bool status = setData(reader);

    TRACE("Leaving");
    return status;
}

/**
 * \brief Read dataset using supplied reader
 *
 * Pipeline information is removed from the resulting 
 * vtkDataSet, so that the reader and its data can be 
 * released
 */
bool DataSet::setData(vtkDataSetReader *reader)
{
    // Force reading data set
    reader->SetLookupTableName("");
    reader->Update();

    _dataSet = reader->GetOutput();
    _dataSet->SetPipelineInformation(NULL);

    _dataSet->GetScalarRange(_dataRange);
    _dataSet->GetBounds(_bounds);

    TRACE("DataSet class: %s", _dataSet->GetClassName());
    TRACE("Scalar Range: %.12e, %.12e", _dataRange[0], _dataRange[1]);
    return true;
}

/**
 * \brief Set DataSet from existing vtkDataSet object
 *
 * Pipeline information is removed from the supplied vtkDataSet
 */
bool DataSet::setData(vtkDataSet *ds)
{
    _dataSet = ds;
    _dataSet->SetPipelineInformation(NULL);
    _dataSet->GetScalarRange(_dataRange);
    _dataSet->GetBounds(_bounds);

    TRACE("DataSet class: %s", _dataSet->GetClassName());
    TRACE("Scalar Range: %.12e, %.12e", _dataRange[0], _dataRange[1]);
    return true;
}

/**
 * \brief Copy an existing vtkDataSet object
 *
 * Pipeline information is not copied from the supplied vtkDataSet
 * into this DataSet, but pipeline information in the supplied 
 * vtkDataSet is not removed
 */
vtkDataSet *DataSet::copyData(vtkDataSet *ds)
{
    if (vtkPolyData::SafeDownCast(ds) != NULL) {
        _dataSet = vtkSmartPointer<vtkPolyData>::New();
        _dataSet->DeepCopy(vtkPolyData::SafeDownCast(ds));
    } else if (vtkStructuredPoints::SafeDownCast(ds) != NULL) {
        _dataSet = vtkSmartPointer<vtkStructuredPoints>::New();
        _dataSet->DeepCopy(vtkStructuredPoints::SafeDownCast(ds));
    } else if (vtkStructuredGrid::SafeDownCast(ds) != NULL) {
        _dataSet = vtkSmartPointer<vtkStructuredGrid>::New();
        _dataSet->DeepCopy(vtkStructuredGrid::SafeDownCast(ds));
    } else if (vtkRectilinearGrid::SafeDownCast(ds) != NULL) {
        _dataSet = vtkSmartPointer<vtkRectilinearGrid>::New();
        _dataSet->DeepCopy(vtkRectilinearGrid::SafeDownCast(ds));
    } else if (vtkUnstructuredGrid::SafeDownCast(ds) != NULL) {
        _dataSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
        _dataSet->DeepCopy(vtkUnstructuredGrid::SafeDownCast(ds));
    } else {
        ERROR("Unknown data type");
        return NULL;
    }
    _dataSet->GetScalarRange(_dataRange);
    _dataSet->GetBounds(_bounds);

    TRACE("DataSet class: %s", _dataSet->GetClassName());
    TRACE("Scalar Range: %.12e, %.12e", _dataRange[0], _dataRange[1]);
    return _dataSet;
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
