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
    DataSet(const std::string& name);
    virtual ~DataSet();

    bool setDataFile(const char *filename);

    bool setData(char *data, int nbytes);

    bool setData(vtkDataSetReader *reader);

    bool setData(vtkDataSet *ds);

    vtkDataSet *copyData(vtkDataSet *ds);

    bool is2D() const;

    const std::string& getName() const;

    vtkDataSet *getVtkDataSet();

    const char *getVtkType() const;

    bool setActiveScalar(const char *name);

    bool setActiveVector(const char *name);

    void getDataRange(double minmax[2]) const;

    void getDataRange(double minmax[2], const char *fieldName) const;

    void getVectorMagnitudeRange(double minmax[2]) const;

    void getVectorComponentRange(double minmax[2], int component) const;

    void getBounds(double bounds[6]) const;

    double getDataValue(double x, double y, double z) const;

    void setVisibility(bool state);

    bool getVisibility() const;

private:
    DataSet();

    std::string _name;
    vtkSmartPointer<vtkDataSet> _dataSet;
    bool _visible;
};

}
}

#endif
