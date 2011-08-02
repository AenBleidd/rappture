/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_CONTOUR2D_H__
#define __RAPPTURE_VTKVIS_CONTOUR2D_H__

#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>

#include <vector>

#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief 2D Contour lines (isolines)
 */
class Contour2D : public VtkGraphicsObject {
public:
    Contour2D();
    virtual ~Contour2D();

    virtual const char *getClassName() const
    {
        return "Contour2D";
    }

    virtual void setDataSet(DataSet *dataset);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setContours(int numContours);

    void setContours(int numContours, double range[2]);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

private:
    virtual void initProp();
    virtual void update();

    int _numContours;
    std::vector<double> _contours;
    double _dataRange[2];

    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
};

}
}

#endif
