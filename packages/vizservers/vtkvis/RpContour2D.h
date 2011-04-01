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

#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief 2D Contour lines (isolines)
 */
class Contour2D {
public:
    Contour2D();
    virtual ~Contour2D();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkActor *getActor();

    void setContours(int numContours);

    void setContours(int numContours, double range[2]);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setVisibility(bool state);

    bool getVisibility() const;

    void setOpacity(double opacity);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    void initActor();
    void update();

    DataSet *_dataSet;

    int _numContours;
    std::vector<double> _contours;
    double _dataRange[2];

    float _edgeColor[3];
    float _edgeWidth;
    double _opacity;
    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
    vtkSmartPointer<vtkActor> _contourActor;
};

}
}

#endif
