/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstring>

#include <vtkCharArray.h>
#include <vtkDataSetReader.h>
#include <vtkPolyData.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkLookupTable.h>

#include "RpVtkDataSet.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

DataSet::DataSet(const std::string& name) :
    _name(name),
    _visible(true)
{
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
    reader->ReadAllFieldsOn();
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
    reader->ReadAllFieldsOn();

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

    TRACE("DataSet class: %s", _dataSet->GetClassName());
#ifdef WANT_TRACE
    double dataRange[2];
    getDataRange(dataRange);
    double bounds[6];
    getBounds(bounds);
#endif
    TRACE("Scalar Range: %.12e, %.12e", dataRange[0], dataRange[1]);
    TRACE("DataSet bounds: %g %g %g %g %g %g",
          bounds[0], bounds[1],
          bounds[2], bounds[3],
          bounds[4], bounds[5]);
    TRACE("Points: %d Cells: %d", _dataSet->GetNumberOfPoints(), _dataSet->GetNumberOfCells());
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

    TRACE("DataSet class: %s", _dataSet->GetClassName());
#ifdef WANT_TRACE
    double dataRange[2];
    getDataRange(dataRange);
    double bounds[6];
    getBounds(bounds);
#endif
    TRACE("Scalar Range: %.12e, %.12e", dataRange[0], dataRange[1]);
    TRACE("DataSet bounds: %g %g %g %g %g %g",
          bounds[0], bounds[1],
          bounds[2], bounds[3],
          bounds[4], bounds[5]);
    TRACE("Points: %d Cells: %d", _dataSet->GetNumberOfPoints(), _dataSet->GetNumberOfCells());
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

    TRACE("DataSet class: %s", _dataSet->GetClassName());
#ifdef WANT_TRACE
    double dataRange[2];
    getDataRange(dataRange);
    double bounds[6];
    getBounds(bounds);
#endif    
    TRACE("Scalar Range: %.12e, %.12e", dataRange[0], dataRange[1]);
    TRACE("DataSet bounds: %g %g %g %g %g %g",
          bounds[0], bounds[1],
          bounds[2], bounds[3],
          bounds[4], bounds[5]);
    return _dataSet;
}

/**
 * \brief Does DataSet lie in the XY plane (z = 0)
 */
bool DataSet::is2D() const
{
    double bounds[6];
    getBounds(bounds);
    return (bounds[4] == 0. && bounds[4] == bounds[5]);
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
const char *DataSet::getVtkType() const
{
    return _dataSet->GetClassName();
}

/**
 * \brief Set the ative scalar array to the named field
 */
bool DataSet::setActiveScalar(const char *name)
{
    bool found = false;
    if (_dataSet != NULL) {
        if (_dataSet->GetPointData() != NULL) {
            if (_dataSet->GetPointData()->SetActiveScalars(name) >= 0)
                found = true;
        }
        if (_dataSet->GetCellData() != NULL) {
            if (_dataSet->GetCellData()->SetActiveScalars(name) >= 0)
                found = true;
        }
    }
    return found;
}

/**
 * \brief Set the ative vector array to the named field
 */
bool DataSet::setActiveVector(const char *name)
{
    bool found = false;
    if (_dataSet != NULL) {
        if (_dataSet->GetPointData() != NULL) {
            if (_dataSet->GetPointData()->SetActiveVectors(name) >= 0)
                found = true;
        }
        if (_dataSet->GetCellData() != NULL) {
            if (_dataSet->GetCellData()->SetActiveVectors(name) >= 0)
                found = true;
        }
    }
    return found;
}

/**
 * \brief Get the range of scalar values in the DataSet
 */
void DataSet::getDataRange(double minmax[2]) const
{
    _dataSet->GetScalarRange(minmax);
}

/**
 * \brief Get the range of scalar values (or vector magnitudes) for
 * the named field in the DataSet
 */
void DataSet::getDataRange(double minmax[2], const char *fieldName) const
{
    if (_dataSet == NULL)
        return;
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetArray(fieldName) != NULL) {
        _dataSet->GetPointData()->GetArray(fieldName)->GetRange(minmax, -1);
    } else if (_dataSet->GetCellData() != NULL &&
        _dataSet->GetCellData()->GetArray(fieldName) != NULL) {
        _dataSet->GetCellData()->GetArray(fieldName)->GetRange(minmax, -1);
    } else if (_dataSet->GetFieldData() != NULL &&
        _dataSet->GetFieldData()->GetArray(fieldName) != NULL) {
        _dataSet->GetFieldData()->GetArray(fieldName)->GetRange(minmax, -1);
    }
}

/**
 * \brief Get the range of vector magnitudes in the DataSet
 */
void DataSet::getVectorMagnitudeRange(double minmax[2]) const
{
    if (_dataSet == NULL) 
        return;
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetVectors() != NULL) {
        _dataSet->GetPointData()->GetVectors()->GetRange(minmax, -1);
    } else if (_dataSet->GetCellData() != NULL &&
               _dataSet->GetCellData()->GetVectors() != NULL) {
        _dataSet->GetCellData()->GetVectors()->GetRange(minmax, -1);
    }
}

/**
 * \brief Get the range of a vector component in the DataSet
 */
void DataSet::getVectorComponentRange(double minmax[2], int component) const
{
    if (_dataSet == NULL)
        return;
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetVectors() != NULL) {
        _dataSet->GetPointData()->GetVectors()->GetRange(minmax, component);
    } else if (_dataSet->GetCellData() != NULL &&
               _dataSet->GetCellData()->GetVectors() != NULL) {
        _dataSet->GetCellData()->GetVectors()->GetRange(minmax, component);
    }
}

/**
 * \brief Get the bounds the DataSet
 */
void DataSet::getBounds(double bounds[6]) const
{
    _dataSet->GetBounds(bounds);
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 *
 * \return the value of the nearest point or 0 if no scalar data available
 */
double DataSet::getDataValue(double x, double y, double z) const
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
