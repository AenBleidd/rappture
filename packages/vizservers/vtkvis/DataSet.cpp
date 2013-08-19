/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>
#include <cstring>
#include <cfloat>
#include <cmath>

#include <vtkCharArray.h>
#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>
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
#include <vtkExtractUnstructuredGrid.h>

#include "DataSet.h"
#include "Trace.h"

using namespace VtkVis;

DataSet::DataSet(const std::string& name) :
    _name(name),
    _visible(true),
    _opacity(1),
    _cellSizeAverage(0)
{
    _cellSizeRange[0] = -1;
    _cellSizeRange[1] = -1;
}

DataSet::~DataSet()
{
}

/**
 * \brief Set opacity of DataSet outline
 *
 * This method is used for record-keeping and opacity of the
 * DataSet bounds outline.  The renderer controls opacity 
 * of other related graphics objects.
 */
void DataSet::setOpacity(double opacity)
{
    _opacity = opacity;
}

/**
 * \brief Set visibility flag in DataSet
 *
 * This method is used for record-keeping and visibility of the
 * DataSet bounds outline.  The renderer controls visibility 
 * of other related graphics objects.
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

void DataSet::writeDataFile(const char *filename)
{
    if (_dataSet == NULL)
        return;
    
    vtkSmartPointer<vtkDataSetWriter> writer = vtkSmartPointer<vtkDataSetWriter>::New();

    writer->SetFileName(filename);
#ifdef USE_VTK6
    writer->SetInputData(_dataSet);
#else
    writer->SetInput(_dataSet);
#endif
    writer->Write();
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
    reader->ReadAllTCoordsOn();
    reader->ReadAllScalarsOn();
    reader->ReadAllColorScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllTensorsOn();
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
    reader->ReadAllTCoordsOn();
    reader->ReadAllScalarsOn();
    reader->ReadAllColorScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllFieldsOn();

    return setData(reader);
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
    if (_dataSet == NULL)
        return false;

    if (vtkUnstructuredGrid::SafeDownCast(_dataSet) != NULL && !isCloud()) {
        vtkSmartPointer<vtkExtractUnstructuredGrid> filter = vtkSmartPointer<vtkExtractUnstructuredGrid>::New();
#ifdef USE_VTK6
        filter->SetInputData(_dataSet);
#else
        filter->SetInput(_dataSet);
#endif
        filter->MergingOn();
        filter->ReleaseDataFlagOn();
        filter->Update();
        _dataSet = filter->GetOutput();
    }

#ifndef USE_VTK6
    _dataSet->SetPipelineInformation(NULL);
#endif
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetScalars() != NULL &&
        _dataSet->GetPointData()->GetScalars()->GetLookupTable() != NULL) {
        USER_ERROR("No lookup table should be specified in VTK data sets");
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
#ifndef USE_VTK6
    _dataSet->SetPipelineInformation(NULL);
#endif

    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetScalars() != NULL &&
        _dataSet->GetPointData()->GetScalars()->GetLookupTable() != NULL) {
        USER_ERROR("No lookup table should be specified in VTK data sets");
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
bool DataSet::is2D(PrincipalPlane *plane, double *offset) const
{
    double bounds[6];
    getBounds(bounds);
    if (bounds[4] == bounds[5]) {
        // Zmin = Zmax, XY plane
        if (plane != NULL) {
            *plane = PLANE_XY;
        }
        if (offset != NULL)
            *offset = bounds[4];
        return true;
    } else if (bounds[0] == bounds[1]) {
        // Xmin = Xmax, ZY plane
        if (plane != NULL) {
            *plane = PLANE_ZY;
        }
        if (offset != NULL)
            *offset = bounds[0];
        return true;
    } else if (bounds[2] == bounds[3]) {
        // Ymin = Ymax, XZ plane
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
PrincipalPlane DataSet::principalPlane() const
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
 * \brief Determines if mesh is a point cloud (has no cells)
 */
bool DataSet::isCloud() const
{
    vtkPolyData *pd = vtkPolyData::SafeDownCast(_dataSet);
    if (pd) {
        // If PolyData, is a cloud if there are no cells other than vertices
        if (pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            return true;
        } else {
            // Has cells
            return false;
        }
    } else {
        return (_dataSet->GetNumberOfCells() == 0);
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
            if (_dataSet->GetPointData()->SetActiveScalars(name) >= 0) {
                TRACE("Set active point data scalars to %s", name);
                found = true;
            }
        }
        if (_dataSet->GetCellData() != NULL) {
            if (_dataSet->GetCellData()->SetActiveScalars(name) >= 0) {
                TRACE("Set active cell data scalars to %s", name);
                found = true;
            }
        }
    }
#ifdef WANT_TRACE
    if (_dataSet->GetPointData() != NULL) {
        if (_dataSet->GetPointData()->GetScalars() != NULL) {
            TRACE("Point data scalars: %s", _dataSet->GetPointData()->GetScalars()->GetName());
        } else {
            TRACE("NULL point data scalars");
        }
    }
    if (_dataSet->GetCellData() != NULL) {
        if (_dataSet->GetCellData()->GetScalars() != NULL) {
            TRACE("Cell data scalars: %s", _dataSet->GetCellData()->GetScalars()->GetName());
        } else {
            TRACE("NULL cell data scalars");
        }
    }
#endif
    return found;
}

/**
 * \brief Get the active scalar array field name
 */
const char *DataSet::getActiveScalarsName() const
{
    if (_dataSet != NULL) {
         if (_dataSet->GetPointData() != NULL &&
             _dataSet->GetPointData()->GetScalars() != NULL) {
            return _dataSet->GetPointData()->GetScalars()->GetName();
        }
#ifdef DEBUG
        TRACE("No point scalars");
#endif
        if (_dataSet->GetCellData() != NULL &&
            _dataSet->GetCellData()->GetScalars() != NULL) {
            return _dataSet->GetCellData()->GetScalars()->GetName();
        }
#ifdef DEBUG
        TRACE("No cell scalars");
#endif
    }
    return NULL;
}

/**
 * \brief Get the active scalar array default attribute type
 */
DataSet::DataAttributeType DataSet::getActiveScalarsType() const
{
    if (_dataSet != NULL) {
         if (_dataSet->GetPointData() != NULL &&
             _dataSet->GetPointData()->GetScalars() != NULL) {
            return POINT_DATA;
        }
#ifdef DEBUG
        TRACE("No point scalars");
#endif
        if (_dataSet->GetCellData() != NULL &&
            _dataSet->GetCellData()->GetScalars() != NULL) {
            return CELL_DATA;
        }
#ifdef DEBUG
        TRACE("No cell scalars");
#endif
    }
    return POINT_DATA;
}

/**
 * \brief Set the ative vector array to the named field
 */
bool DataSet::setActiveVectors(const char *name)
{
    bool found = false;
    if (_dataSet != NULL) {
        if (_dataSet->GetPointData() != NULL) {
            if (_dataSet->GetPointData()->SetActiveVectors(name) >= 0) {
                TRACE("Set active point data vectors to %s", name);
                found = true;
            }
        }
        if (_dataSet->GetCellData() != NULL) {
            if (_dataSet->GetCellData()->SetActiveVectors(name) >= 0) {
                TRACE("Set active cell data vectors to %s", name);
                found = true;
            }
        }
    }

    return found;
}

/**
 * \brief Get the active vector array default attribute type
 */
DataSet::DataAttributeType DataSet::getActiveVectorsType() const
{
    if (_dataSet != NULL) {
         if (_dataSet->GetPointData() != NULL &&
             _dataSet->GetPointData()->GetVectors() != NULL) {
            return POINT_DATA;
        }
#ifdef DEBUG
        TRACE("No point vectors");
#endif
        if (_dataSet->GetCellData() != NULL &&
            _dataSet->GetCellData()->GetVectors() != NULL) {
            return CELL_DATA;
        }
#ifdef DEBUG
        TRACE("No cell vectors");
#endif
    }
    return POINT_DATA;
}

/**
 * \brief Get the active vector array field name
 */
const char *DataSet::getActiveVectorsName() const
{
    if (_dataSet != NULL) {
        if (_dataSet->GetPointData() != NULL &&
            _dataSet->GetPointData()->GetVectors() != NULL) {
            return _dataSet->GetPointData()->GetVectors()->GetName();
        }
#ifdef DEBUG
        TRACE("No point vectors");
#endif
        if (_dataSet->GetCellData() != NULL &&
            _dataSet->GetCellData()->GetVectors() != NULL) {
            return _dataSet->GetCellData()->GetVectors()->GetName();
        }
#ifdef DEBUG
        TRACE("No cell vectors");
#endif
    }
    return NULL;
}

void DataSet::setDefaultArrays()
{
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetScalars() == NULL &&
        _dataSet->GetPointData()->GetNumberOfArrays() > 0) {
        for (int i = 0; i < _dataSet->GetPointData()->GetNumberOfArrays(); i++) {
            if (_dataSet->GetPointData()->GetArray(i) != NULL &&
                _dataSet->GetPointData()->GetArray(i)->GetNumberOfComponents() == 1) {
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
            if (_dataSet->GetPointData()->GetArray(i) != NULL &&
                _dataSet->GetPointData()->GetArray(i)->GetNumberOfComponents() == 3) {
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
            if (_dataSet->GetCellData()->GetArray(i) != NULL &&
                _dataSet->GetCellData()->GetArray(i)->GetNumberOfComponents() == 1) {
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
            if (_dataSet->GetCellData()->GetArray(i) != NULL &&
                _dataSet->GetCellData()->GetArray(i)->GetNumberOfComponents() == 3) {
                TRACE("Setting cell vectors to '%s'", _dataSet->GetCellData()->GetArrayName(i));
                _dataSet->GetCellData()->SetActiveVectors(_dataSet->GetCellData()->GetArrayName(i));
                break;
            }
        }
    }
}

bool DataSet::getFieldInfo(const char *fieldName,
                           DataAttributeType *type,
                           int *numComponents) const
{
    if (_dataSet->GetPointData() != NULL &&
        _dataSet->GetPointData()->GetAbstractArray(fieldName) != NULL) {
        if (type != NULL)
            *type = POINT_DATA;
        if (numComponents != NULL)
            *numComponents = _dataSet->GetPointData()->GetAbstractArray(fieldName)->GetNumberOfComponents();
        return true;
    } else if (_dataSet->GetCellData() != NULL &&
               _dataSet->GetCellData()->GetAbstractArray(fieldName) != NULL) {
        if (type != NULL)
            *type = CELL_DATA;
        if (numComponents != NULL)
            *numComponents = _dataSet->GetCellData()->GetAbstractArray(fieldName)->GetNumberOfComponents();
        return true;
    } else if (_dataSet->GetFieldData() != NULL &&
               _dataSet->GetFieldData()->GetAbstractArray(fieldName) != NULL) {
        if (type != NULL)
            *type = FIELD_DATA;
        if (numComponents != NULL)
            *numComponents = _dataSet->GetFieldData()->GetAbstractArray(fieldName)->GetNumberOfComponents();
        return true;
    }
    return false;
}

bool DataSet::getFieldInfo(const char *fieldName,
                           DataAttributeType type,
                           int *numComponents) const
{
    switch (type) {
    case POINT_DATA:
        if (_dataSet->GetPointData() != NULL &&
            _dataSet->GetPointData()->GetAbstractArray(fieldName) != NULL) {
            if (numComponents != NULL)
                *numComponents = _dataSet->GetPointData()->GetAbstractArray(fieldName)->GetNumberOfComponents();
            return true;
        } else
            return false;
        break;
    case CELL_DATA:
        if (_dataSet->GetCellData() != NULL &&
            _dataSet->GetCellData()->GetAbstractArray(fieldName) != NULL) {
            if (numComponents != NULL)
                *numComponents = _dataSet->GetCellData()->GetAbstractArray(fieldName)->GetNumberOfComponents();
            return true;
        } else
            return false;
        break;
    case FIELD_DATA:
        if (_dataSet->GetFieldData() != NULL &&
            _dataSet->GetFieldData()->GetAbstractArray(fieldName) != NULL) {
            if (numComponents != NULL)
                *numComponents = _dataSet->GetFieldData()->GetAbstractArray(fieldName)->GetNumberOfComponents();
            return true;
        } else
            return false;
        break;
    default:
        ;
    }
    return false;
}

/**
 * \brief Get the list of field names in the DataSet
 * 
 * \param[in,out] names The field names will be appended to this list
 * \param[in] type The DataAttributeType: e.g. POINT_DATA, CELL_DATA
 * \param[in] numComponents Filter list by number of components, -1 means to
 * return all fields regardless of dimension
 */
void DataSet::getFieldNames(std::vector<std::string>& names,
                            DataAttributeType type, int numComponents) const
{
    if (_dataSet == NULL)
        return;
    switch (type) {
    case POINT_DATA:
        if (_dataSet->GetPointData() != NULL) {
            for (int i = 0; i < _dataSet->GetPointData()->GetNumberOfArrays(); i++) {
                if (numComponents == -1 ||
                    (_dataSet->GetPointData()->GetAbstractArray(i) != NULL &&
                     _dataSet->GetPointData()->GetAbstractArray(i)->GetNumberOfComponents() == numComponents)) {
                    names.push_back(_dataSet->GetPointData()->GetArrayName(i));
                }
            }
        }
        break;
    case CELL_DATA:
        if (_dataSet->GetCellData() != NULL) {
            for (int i = 0; i < _dataSet->GetCellData()->GetNumberOfArrays(); i++) {
                if (numComponents == -1 ||
                    (_dataSet->GetCellData()->GetAbstractArray(i) != NULL &&
                     _dataSet->GetCellData()->GetAbstractArray(i)->GetNumberOfComponents() == numComponents)) {
                    names.push_back(_dataSet->GetCellData()->GetArrayName(i));
                }
            }
        }
        break;
    case FIELD_DATA:
        if (_dataSet->GetFieldData() != NULL) {
            for (int i = 0; i < _dataSet->GetFieldData()->GetNumberOfArrays(); i++) {
                if (numComponents == -1 ||
                    (_dataSet->GetFieldData()->GetAbstractArray(i) != NULL &&
                     _dataSet->GetFieldData()->GetAbstractArray(i)->GetNumberOfComponents() == numComponents)) {
                    names.push_back(_dataSet->GetFieldData()->GetArrayName(i));
                }
            }
        }
        break;
    default:
        ERROR("Unknown DataAttributeType %d", type);
    }
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
 * \param[in] type The DataAttributeType
 * \param[in] component The field component, -1 means magnitude
 * \return boolean indicating if field was found
 */
bool DataSet::getDataRange(double minmax[2], const char *fieldName,
                           DataAttributeType type, int component) const
{
    if (_dataSet == NULL)
        return false;
    switch (type) {
    case POINT_DATA:
        if (_dataSet->GetPointData() != NULL &&
            _dataSet->GetPointData()->GetArray(fieldName) != NULL) {
            _dataSet->GetPointData()->GetArray(fieldName)->GetRange(minmax, component);
            return true;
        } else {
            return false;
        }
        break;
    case CELL_DATA:
        if (_dataSet->GetCellData() != NULL &&
            _dataSet->GetCellData()->GetArray(fieldName) != NULL) {
            _dataSet->GetCellData()->GetArray(fieldName)->GetRange(minmax, component);
            return true;
        } else {
            return false;
        }
        break;
    case FIELD_DATA:
        if (_dataSet->GetFieldData() != NULL &&
            _dataSet->GetFieldData()->GetArray(fieldName) != NULL) {
            _dataSet->GetFieldData()->GetArray(fieldName)->GetRange(minmax, component);
            return true;
        } else {
            return false;
        }
        break;
    default:
        ERROR("Unknown DataAttributeType %d", type);
        break;
    }
    return false;
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
    // Check for cached values
    if (_cellSizeRange[0] >= 0.0) {
        minmax[0] = _cellSizeRange[0];
        minmax[1] = _cellSizeRange[1];
        *average = _cellSizeAverage;
        return;
    }

    getCellSizeRange(_dataSet, minmax, average);

    // Save values in cache
    _cellSizeRange[0] = minmax[0];
    _cellSizeRange[1] = minmax[1];
    _cellSizeAverage = *average;
}

/**
 * \brief Get the range of cell AABB diagonal lengths in the DataSet
 */
void DataSet::getCellSizeRange(vtkDataSet *dataSet, double minmax[2], double *average)
{
    if (dataSet == NULL ||
        dataSet->GetNumberOfCells() < 1) {
        minmax[0] = 1;
        minmax[1] = 1;
        *average = 1;
        return;
    }

    minmax[0] = DBL_MAX;
    minmax[1] = -DBL_MAX;

    *average = 0;
    for (int i = 0; i < dataSet->GetNumberOfCells(); i++) {
        double length2 = dataSet->GetCell(i)->GetLength2();
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
    *average = sqrt(*average/((double)dataSet->GetNumberOfCells()));
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 *
 * \param[in] x World x coordinate to probe
 * \param[in] y World y coordinate to probe
 * \param[in] z World z coordinate to probe
 * \param[out] value On success, contains the data value
 * \return boolean indicating success or failure
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

void DataSet::print() const
{
    print(_dataSet);
}

void DataSet::print(vtkDataSet *ds)
{
    if (ds == NULL)
        return;

    TRACE("DataSet class: %s", ds->GetClassName());

    TRACE("DataSet memory: %g MiB", ds->GetActualMemorySize()/1024.);

    double bounds[6];
    ds->GetBounds(bounds);

    // Topology
    TRACE("DataSet bounds: %g %g %g %g %g %g",
          bounds[0], bounds[1],
          bounds[2], bounds[3],
          bounds[4], bounds[5]);
    TRACE("Points: %d Cells: %d", ds->GetNumberOfPoints(), ds->GetNumberOfCells());

    double dataRange[2];
    if (ds->GetPointData() != NULL) {
        TRACE("PointData arrays: %d", ds->GetPointData()->GetNumberOfArrays());
        for (int i = 0; i < ds->GetPointData()->GetNumberOfArrays(); i++) {
            if (ds->GetPointData()->GetArray(i) != NULL) {
                ds->GetPointData()->GetArray(i)->GetRange(dataRange, -1);
                TRACE("PointData[%d]: '%s' comp:%d, (%g,%g)", i,
                      ds->GetPointData()->GetArrayName(i),
                      ds->GetPointData()->GetAbstractArray(i)->GetNumberOfComponents(),
                      dataRange[0], dataRange[1]);
            } else {
                TRACE("PointData[%d]: '%s' comp:%d", i,
                      ds->GetPointData()->GetArrayName(i),
                      ds->GetPointData()->GetAbstractArray(i)->GetNumberOfComponents());
            }
        }
        if (ds->GetPointData()->GetScalars() != NULL) {
            TRACE("Active point scalars: %s", ds->GetPointData()->GetScalars()->GetName());
        }
        if (ds->GetPointData()->GetVectors() != NULL) {
            TRACE("Active point vectors: %s", ds->GetPointData()->GetVectors()->GetName());
        }
    }
    if (ds->GetCellData() != NULL) {
        TRACE("CellData arrays: %d", ds->GetCellData()->GetNumberOfArrays());
        for (int i = 0; i < ds->GetCellData()->GetNumberOfArrays(); i++) {
            if (ds->GetCellData()->GetArray(i) != NULL) {
                ds->GetCellData()->GetArray(i)->GetRange(dataRange, -1);
                TRACE("CellData[%d]: '%s' comp:%d, (%g,%g)", i,
                      ds->GetCellData()->GetArrayName(i),
                      ds->GetCellData()->GetAbstractArray(i)->GetNumberOfComponents(),
                      dataRange[0], dataRange[1]);
            } else {
                TRACE("CellData[%d]: '%s' comp:%d", i,
                      ds->GetCellData()->GetArrayName(i),
                      ds->GetCellData()->GetAbstractArray(i)->GetNumberOfComponents());
            }
        }
        if (ds->GetCellData()->GetScalars() != NULL) {
            TRACE("Active cell scalars: %s", ds->GetCellData()->GetScalars()->GetName());
        }
        if (ds->GetCellData()->GetVectors() != NULL) {
            TRACE("Active cell vectors: %s", ds->GetCellData()->GetVectors()->GetName());
        }
    }
    if (ds->GetFieldData() != NULL) {
        TRACE("FieldData arrays: %d", ds->GetFieldData()->GetNumberOfArrays());
        for (int i = 0; i < ds->GetFieldData()->GetNumberOfArrays(); i++) {
            if (ds->GetFieldData()->GetArray(i) != NULL) {
                ds->GetFieldData()->GetArray(i)->GetRange(dataRange, -1);
                TRACE("FieldData[%d]: '%s' comp:%d, tuples:%d (%g,%g)", i,
                      ds->GetFieldData()->GetArrayName(i),
                      ds->GetFieldData()->GetAbstractArray(i)->GetNumberOfComponents(),
                      ds->GetFieldData()->GetAbstractArray(i)->GetNumberOfTuples(),
                      dataRange[0], dataRange[1]);
            } else {
                TRACE("FieldData[%d]: '%s' comp:%d, tuples:%d", i,
                      ds->GetFieldData()->GetArrayName(i),
                      ds->GetFieldData()->GetAbstractArray(i)->GetNumberOfComponents(),
                      ds->GetFieldData()->GetAbstractArray(i)->GetNumberOfTuples());
            }
        }
    }
}

