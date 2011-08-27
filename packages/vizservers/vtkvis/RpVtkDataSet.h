/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_DATASET_H__
#define __RAPPTURE_VTKVIS_DATASET_H__

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkDataSetReader.h>

#include <string>
#include <vector>

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK DataSet wrapper
 */
class DataSet {
public:
    enum PrincipalPlane {
        PLANE_XY,
        PLANE_ZY,
        PLANE_XZ
    };
    DataSet(const std::string& name);
    virtual ~DataSet();

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

    bool setActiveVectors(const char *name);

    void getScalarRange(double minmax[2]) const;

    void getDataRange(double minmax[2], const char *fieldName, int component = -1) const;

    void getVectorRange(double minmax[2], int component = -1) const;

    void getBounds(double bounds[6]) const;

    void getCellSizeRange(double minmax[6], double *average) const;

    bool getScalarValue(double x, double y, double z, double *value) const;

    bool getVectorValue(double x, double y, double z, double vector[3]) const;

    void setVisibility(bool state);

    bool getVisibility() const;

private:
    DataSet();

    void setDefaultArrays();
    void print() const;

    std::string _name;
    vtkSmartPointer<vtkDataSet> _dataSet;
    bool _visible;
};

}
}

#endif
