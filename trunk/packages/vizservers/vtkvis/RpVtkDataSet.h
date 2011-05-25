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

    bool setData(char *data, int nbytes);

    bool setData(vtkDataSet *ds);

    bool setData(vtkDataSetReader *reader);

    bool setDataFile(const char *filename);

    bool is2D() const;

    const std::string& getName() const;

    vtkDataSet *getVtkDataSet();

    const char *getVtkType();

    void getDataRange(double minmax[2]);

    double getDataValue(double x, double y, double z);

    void setVisibility(bool state);

    bool getVisibility() const;

private:
    DataSet();

    std::string _name;
    vtkSmartPointer<vtkDataSet> _dataSet;
    double _dataRange[2];
    double _bounds[6];
    bool _visible;
};

}
}

#endif
