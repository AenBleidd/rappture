/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_STREAMLINES_H__
#define __RAPPTURE_VTKVIS_STREAMLINES_H__

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkPlaneCollection.h>
#include <vtkStreamTracer.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkPolyDataMapper.h>
#include <vtkLookupTable.h>
#include <vtkPropAssembly.h>

#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Streamline visualization of vector fields
 */
class Streamlines {
public:
    enum LineType {
        LINES,
        TUBES,
        RIBBONS
    };

    Streamlines();
    virtual ~Streamlines();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void setSeedToRandomPoints(int numPoints);

    void setSeedToRake(double start[3], double end[3], int numPoints);

    void setSeedToPolygon(double center[3], double normal[3],
                          double radius, int numSides);

    void setMaxPropagation(double length);

    void setLineTypeToLines();

    void setLineTypeToTubes(int numSides, double radius);

    void setLineTypeToRibbons(double width, double angle);

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setOpacity(double opacity);

    double getOpacity();

    void setVisibility(bool state);

    void setSeedVisibility(bool state);

    bool getVisibility();

    void setEdgeVisibility(bool state);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setSeedColor(float color[3]);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    void initProp();
    void update();

    static void getRandomPoint(double pt[3], const double bounds[6]);
    static void getRandomCellPt(vtkDataSet *ds, double pt[3]);

    DataSet *_dataSet;

    LineType _lineType;
    float _edgeColor[3];
    float _edgeWidth;
    float _seedColor[3];
    double _opacity;
    bool _lighting;
    bool _seedVisible;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkActor> _prop;
    vtkSmartPointer<vtkActor> _seedActor;
    vtkSmartPointer<vtkPropAssembly> _props;
    vtkSmartPointer<vtkStreamTracer> _streamTracer;
    vtkSmartPointer<vtkPolyDataAlgorithm> _lineFilter;
    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
    vtkSmartPointer<vtkPolyDataMapper> _seedMapper;
};

}
}

#endif
