/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_CONTOUR3D_H__
#define __RAPPTURE_VTKVIS_CONTOUR3D_H__

#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>

#include <vector>

#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief 3D Contour isosurfaces (geometry)
 */
class Contour3D : public VtkGraphicsObject {
public:
    Contour3D();
    virtual ~Contour3D();

    virtual const char *getClassName() const
    {
        return "Contour3D";
    }

    virtual void setDataSet(DataSet *dataset);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setContours(int numContours);

    void setContours(int numContours, double range[2]);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

private:
    virtual void initProp();
    virtual void update();

    int _numContours;
    std::vector<double> _contours;
    double _dataRange[2];

    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
};

}
}

#endif
