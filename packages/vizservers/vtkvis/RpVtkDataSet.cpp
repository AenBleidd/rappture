/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>
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
    _visible(true),
    _cellSizeAverage(0)
{
    _cellSizeRange[0] = -1;
    _cellSizeRange[1] = -1;
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

    return setData(reader);
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

void DataSet::setDefaultArrays()
{
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetScalars() == NULL &&
        _dataSet->GetPointData()->GetNumberOfArrays() > 0) {
        for (int i = 0; i < _dataSet->GetPointData()->GetNumberOfArrays(); i++) {
            if (_dataSet->GetPointData()->GetArray(i)->GetNumberOfComponents() == 1) {
                TRACE("Setting point scalars to '%s'", _dataSet->GetPointData()->GetArrayName(i));
                _dataSet->GetPointData()->SetActiveScalars(_dataSet->GetPointData()->GetArrayName(i));
                break;
            }
        }
    }
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetVectors() == NULL &&
        _dataSet->GetPointData()->GetNumberOfArrays() > 0) {
        for (int i = 0; i < _dataSet->GetPointData()->GetNumberOfArrays(); i++) {
            if (_dataSet->GetPointData()->GetArray(i)->GetNumberOfComponents() == 3) {
                TRACE("Setting point vectors to '%s'", _dataSet->GetPointData()->GetArrayName(i));
                _dataSet->GetPointData()->SetActiveVectors(_dataSet->GetPointData()->GetArrayName(i));
                break;
            }
        }
    }
    if (_dataSet->GetCellData() != NULL &&
        _dataSet->GetCellData()->GetScalars() == NULL &&
        _dataSet->GetCellData()->GetNumberOfArrays() > 0) {
        for (int i = 0; i < _dataSet->GetCellData()->GetNumberOfArrays(); i++) {
            if (_dataSet->GetCellData()->GetArray(i)->GetNumberOfComponents() == 1) {
                TRACE("Setting cell scalars to '%s'", _dataSet->GetCellData()->GetArrayName(i));
                _dataSet->GetCellData()->SetActiveScalars(_dataSet->GetCellData()->GetArrayName(i));
                break;
            }
        }
    }
    if (_dataSet->GetCellData() != NULL &&
        _dataSet->GetCellData()->GetVectors() == NULL &&
        _dataSet->GetCellData()->GetNumberOfArrays() > 0) {
        for (int i = 0; i < _dataSet->GetCellData()->GetNumberOfArrays(); i++) {
            if (_dataSet->GetCellData()->GetArray(i)->GetNumberOfComponents() == 3) {
                TRACE("Setting cell vectors to '%s'", _dataSet->GetCellData()->GetArrayName(i));
                _dataSet->GetCellData()->SetActiveVectors(_dataSet->GetCellData()->GetArrayName(i));
                break;
            }
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
    TRACE("Enter");
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

    setDefaultArrays();

#ifdef WANT_TRACE
    print();
#endif
    TRACE("Leave");
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

    setDefaultArrays();

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
bool DataSet::isXY() const
{
    double bounds[6];
    getBounds(bounds);
    return (bounds[4] == 0. && bounds[4] == bounds[5]);
}

/**
 * \brief Returns the dimensionality of the AABB
 */
int DataSet::numDimensions() const
{
    double bounds[6];
    getBounds(bounds);
    int numDims = 0;
    if (bounds[0] != bounds[1])
        numDims++;
    if (bounds[2] != bounds[3])
        numDims++;
    if (bounds[4] != bounds[5])
        numDims++;

    return numDims;
}

/**
 * \brief Determines if DataSet lies in a principal axis plane
 * and if so, returns the plane normal and offset from origin
 */
bool DataSet::is2D(DataSet::PrincipalPlane *plane, double *offset) const
{
    double bounds[6];
    getBounds(bounds);
    if (bounds[4] == bounds[5]) {
        // Z = 0, XY plane
        if (plane != NULL) {
            *plane = PLANE_XY;
        }
        if (offset != NULL)
            *offset = bounds[4];
        return true;
    } else if (bounds[0] == bounds[1]) {
        // X = 0, ZY plane
        if (plane != NULL) {
            *plane = PLANE_ZY;
        }
        if (offset != NULL)
            *offset = bounds[0];
        return true;
    } else if (bounds[2] == bounds[3]) {
        // Y = 0, XZ plane
        if (plane != NULL) {
            *plane = PLANE_XZ;
         }
        if (offset != NULL)
            *offset = bounds[2];
        return true;
    }
    return false;
}

/**
 * \brief Determines a principal plane with the
 * largest two dimensions of the AABB
 */
DataSet::PrincipalPlane DataSet::principalPlane() const
{
    double bounds[6];
    getBounds(bounds);
    double xlen = bounds[1] - bounds[0];
    double ylen = bounds[3] - bounds[2];
    double zlen = bounds[5] - bounds[4];
    if (zlen <= xlen && zlen <= ylen) {
        return PLANE_XY;
    } else if (xlen <= ylen && xlen <= zlen) {
        return PLANE_ZY;
    } else {
        return PLANE_XZ;
    }
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
    if (_dataSet == NULL)
        return;
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetScalars() != NULL) {
        _dataSet->GetPointData()->GetScalars()->GetRange(minmax, -1);
    } else if (_dataSet->GetCellData() != NULL &&
               _dataSet->GetCellData()->GetScalars() != NULL) {
        _dataSet->GetCellData()->GetScalars()->GetRange(minmax, -1);
    }
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
void DataSet::getCellSizeRange(double minmax[2], double *average)
{
    if (_dataSet == NULL ||
        _dataSet->GetNumberOfCells() < 1) {
        minmax[0] = 1;
        minmax[1] = 1;
        *average = 1;
        return;
    }

    if (_cellSizeRange[0] >= 0.0) {
        minmax[0] = _cellSizeRange[0];
        minmax[1] = _cellSizeRange[1];
        *average = _cellSizeAverage;
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
    _cellSizeRange[0] = minmax[0];
    _cellSizeRange[1] = minmax[1];
    _cellSizeAverage = *average;
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 *
 * \return the value of the nearest point or 0 if no scalar data available
 */
bool DataSet::getScalarValue(double x, double y, double z, double *value) const
{
    if (_dataSet == NULL)
        return false;
    if (_dataSet->GetPointData() == NULL ||
        _dataSet->GetPointData()->GetScalars() == NULL) {
        return false;
    }
    vtkIdType pt = _dataSet->FindPoint(x, y, z);
    if (pt < 0)
        return false;
    *value = _dataSet->GetPointData()->GetScalars()->GetComponent(pt, 0);
    return true;
}

/**
 * \brief Get nearest vector data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 *
 * \param[in] x World x coordinate to probe
 * \param[in] y World y coordinate to probe
 * \param[in] z World z coordinate to probe
 * \param[out] vector On success, contains the data values
 * \return boolean indicating success or failure
 */
bool DataSet::getVectorValue(double x, double y, double z, double vector[3]) const
{
    if (_dataSet == NULL)
        return false;
    if (_dataSet->GetPointData() == NULL ||
        _dataSet->GetPointData()->GetVectors() == NULL) {
        return false;
    }
    vtkIdType pt = _dataSet->FindPoint(x, y, z);
    if (pt < 0)
        return false;
    assert(_dataSet->GetPointData()->GetVectors()->GetNumberOfComponents() == 3);
    _dataSet->GetPointData()->GetVectors()->GetTuple(pt, vector);
    return true;
}
