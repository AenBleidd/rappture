/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_DATASET_H
#define VTKVIS_DATASET_H

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkDataSetReader.h>

#include <string>
#include <vector>

#include "RpTypes.h"
#include "Trace.h"

namespace VtkVis {

/**
 * \brief VTK DataSet wrapper
 */
class DataSet {
public:
    enum DataAttributeType {
        POINT_DATA,
        CELL_DATA,
        FIELD_DATA
    };
    DataSet(const std::string& name);
    virtual ~DataSet();

    void writeDataFile(const char *filename);

    bool setDataFile(const char *filename);

    bool setData(char *data, int nbytes);

    bool setData(vtkDataSetReader *reader);

    bool setData(vtkDataSet *ds);

    vtkDataSet *copyData(vtkDataSet *ds);

    bool isXY() const;

    int numDimensions() const;

    bool is2D(PrincipalPlane *plane = NULL, double *offset = NULL) const;

    PrincipalPlane principalPlane() const;

    const std::string& getName() const;

    vtkDataSet *getVtkDataSet();

    const char *getVtkType() const;

    bool setActiveScalars(const char *name);

    const char *getActiveScalarsName() const;

    DataAttributeType getActiveScalarsType() const;

    bool setActiveVectors(const char *name);

    const char *getActiveVectorsName() const;

    DataAttributeType getActiveVectorsType() const;

    bool hasField(const char *fieldName) const
    {
        return getFieldInfo(fieldName, NULL, NULL);
    }

    bool hasField(const char *fieldName, DataAttributeType type) const
    {
        return getFieldInfo(fieldName, type, NULL);
    }

    bool getFieldInfo(const char *fieldName, DataAttributeType *type, int *numComponents) const;

    bool getFieldInfo(const char *fieldName, DataAttributeType type, int *numComponents) const;

    void getFieldNames(std::vector<std::string>& names,
                       DataAttributeType type, int numComponents = -1) const;

    bool getDataRange(double minmax[2], const char *fieldName,
                      DataAttributeType type, int component = -1) const;

    void getScalarRange(double minmax[2]) const;

    void getVectorRange(double minmax[2], int component = -1) const;

    void getBounds(double bounds[6]) const;

    void getCellSizeRange(double minmax[2], double *average);

    bool getScalarValue(double x, double y, double z, double *value) const;

    bool getVectorValue(double x, double y, double z, double vector[3]) const;

    void setOpacity(double opacity);

    /**
     * \brief Get the opacity setting for the DataSet
     *
     * This method is used for record-keeping.  The renderer controls
     * the visibility of related graphics objects.
     */
    inline double getOpacity()
    {
        return _opacity;
    }

    void setVisibility(bool state);

    bool getVisibility() const;

    static void print(vtkDataSet *ds);

private:
    DataSet();

    void setDefaultArrays();
    void print() const;

    std::string _name;
    vtkSmartPointer<vtkDataSet> _dataSet;
    bool _visible;
    double _opacity;
    double _cellSizeRange[2];
    double _cellSizeAverage;
};

}

#endif
