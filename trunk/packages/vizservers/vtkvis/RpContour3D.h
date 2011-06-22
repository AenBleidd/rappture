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

#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief 3D Contour isosurfaces (geometry)
 */
class Contour3D {
public:
    Contour3D();
    virtual ~Contour3D();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void setContours(int numContours);

    void setContours(int numContours, double range[2]);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setVisibility(bool state);

    bool getVisibility() const;

    void setOpacity(double opacity);

    void setWireframe(bool state);

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setColor(float color[3]);

    void setEdgeVisibility(bool state);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    void initProp();
    void update();

    DataSet *_dataSet;

    int _numContours;
    std::vector<double> _contours;
    double _dataRange[2];

    float _color[3];
    float _edgeColor[3];
    float _edgeWidth;
    double _opacity;
    bool _lighting;

    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
    vtkSmartPointer<vtkActor> _contourActor;
};

}
}

#endif
