/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstring>
#include <cfloat>
#include <cmath>

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
#include <vtkCell.h>
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
    reader->ReadAllNormalsOn();
    //reader->ReadAllTCoordsOn();
    reader->ReadAllScalarsOn();
    //reader->ReadAllColorScalarsOn();
    reader->ReadAllVectorsOn();
    //reader->ReadAllTensorsOn();
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
    reader->ReadAllNormalsOn();
    //reader->ReadAllTCoordsOn();
    reader->ReadAllScalarsOn();
    //reader->ReadAllColorScalarsOn();
    reader->ReadAllVectorsOn();
    //reader->ReadAllTensorsOn();
    reader->ReadAllFieldsOn();

    bool status = setData(reader);

    TRACE("Leaving");
    return status;
}

void DataSet::print() const
{
    TRACE("DataSet class: %s", _dataSet->GetClassName());

    double bounds[6];
    getBounds(bounds);

    // Topology
    TRACE("DataSet bounds: %g %g %g %g %g %g",
          bounds[0], bounds[1],
          bounds[2], bounds[3],
          bounds[4], bounds[5]);
    TRACE("Points: %d Cells: %d", _dataSet->GetNumberOfPoints(), _dataSet->GetNumberOfCells());

    double dataRange[2];
    if (_dataSet->GetPointData() != NULL) {
        TRACE("PointData arrays: %d", _dataSet->GetPointData()->GetNumberOfArrays());
        for (int i = 0; i < _dataSet->GetPointData()->GetNumberOfArrays(); i++) {
            _dataSet->GetPointData()->GetArray(i)->GetRange(dataRange, -1);
            TRACE("PointData[%d]: '%s' comp:%d, (%g,%g)", i,
                  _dataSet->GetPointData()->GetArrayName(i),
                  _dataSet->GetPointData()->GetArray(i)->GetNumberOfComponents(),
                  dataRange[0], dataRange[1]);
        }
    }
    if (_dataSet->GetCellData() != NULL) {
        TRACE("CellData arrays: %d", _dataSet->GetCellData()->GetNumberOfArrays());
        for (int i = 0; i < _dataSet->GetCellData()->GetNumberOfArrays(); i++) {
            _dataSet->GetCellData()->GetArray(i)->GetRange(dataRange, -1);
            TRACE("CellData[%d]: '%s' comp:%d, (%g,%g)", i,
                  _dataSet->GetCellData()->GetArrayName(i),
                  _dataSet->GetCellData()->GetArray(i)->GetNumberOfComponents(),
                  dataRange[0], dataRange[1]);
        }
    }
    if (_dataSet->GetFieldData() != NULL) {
        TRACE("FieldData arrays: %d", _dataSet->GetFieldData()->GetNumberOfArrays());
        for (int i = 0; i < _dataSet->GetFieldData()->GetNumberOfArrays(); i++) {
            _dataSet->GetFieldData()->GetArray(i)->GetRange(dataRange, -1);
            TRACE("FieldData[%d]: '%s' comp:%d, tuples:%d (%g,%g)", i,
                  _dataSet->GetFieldData()->GetArrayName(i),
                  _dataSet->GetFieldData()->GetArray(i)->GetNumberOfComponents(),
                  _dataSet->GetFieldData()->GetArray(i)->GetNumberOfTuples(),
                  dataRange[0], dataRange[1]);
        }
    }
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

    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetScalars() != NULL &&
        _dataSet->GetPointData()->GetScalars()->GetLookupTable() != NULL) {
        ERROR("No lookup table should be specified in DataSets");
    }

#ifdef WANT_TRACE
    print();
#endif
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

    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetScalars() != NULL &&
        _dataSet->GetPointData()->GetScalars()->GetLookupTable() != NULL) {
        ERROR("No lookup table should be specified in DataSets");
    }

#ifdef WANT_TRACE
    print();
#endif
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

#ifdef WANT_TRACE
    print();
#endif    
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
bool DataSet::setActiveScalars(const char *name)
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
bool DataSet::setActiveVectors(const char *name)
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
void DataSet::getScalarRange(double minmax[2]) const
{
    _dataSet->GetScalarRange(minmax);
}

/**
 * \brief Get the range of a vector component in the DataSet
 *
 * \param[out] minmax The data range
 * \param[in] component The field component, -1 means magnitude
 */
void DataSet::getVectorRange(double minmax[2], int component) const
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
 * \brief Get the range of values for the named field in the DataSet
 *
 * \param[out] minmax The data range
 * \param[in] fieldName The array name
 * \param[in] component The field component, -1 means magnitude
 */
void DataSet::getDataRange(double minmax[2], const char *fieldName, int component) const
{
    if (_dataSet == NULL)
        return;
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetArray(fieldName) != NULL) {
        _dataSet->GetPointData()->GetArray(fieldName)->GetRange(minmax, component);
    } else if (_dataSet->GetCellData() != NULL &&
        _dataSet->GetCellData()->GetArray(fieldName) != NULL) {
        _dataSet->GetCellData()->GetArray(fieldName)->GetRange(minmax, component);
    } else if (_dataSet->GetFieldData() != NULL &&
        _dataSet->GetFieldData()->GetArray(fieldName) != NULL) {
        _dataSet->GetFieldData()->GetArray(fieldName)->GetRange(minmax, component);
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
 * \brief Get the range of cell AABB diagonal lengths in the DataSet
 */
void DataSet::getCellSizeRange(double minmax[6], double *average) const
{
    if (_dataSet == NULL ||
        _dataSet->GetNumberOfCells() < 1) {
        minmax[0] = 1;
        minmax[1] = 1;
        *average = 1;
        return;
    }

    minmax[0] = DBL_MAX;
    minmax[1] = -DBL_MAX;

    *average = 0;
    for (int i = 0; i < _dataSet->GetNumberOfCells(); i++) {
        double length2 = _dataSet->GetCell(i)->GetLength2();
        if (length2 < minmax[0])
            minmax[0] = length2;
        if (length2 > minmax[1])
            minmax[1] = length2;
        *average += length2;
    }
    if (minmax[0] == DBL_MAX)
        minmax[0] = 1;
    if (minmax[1] == -DBL_MAX)
        minmax[1] = 1;

    minmax[0] = sqrt(minmax[0]);
    minmax[1] = sqrt(minmax[1]);
    *average = sqrt(*average/((double)_dataSet->GetNumberOfCells()));
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
