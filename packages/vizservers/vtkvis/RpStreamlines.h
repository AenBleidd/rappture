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
#include <vtkAssembly.h>

#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Streamline visualization of vector fields
 *
 * The DataSet must contain vectors
 */
class Streamlines : public VtkGraphicsObject {
public:
    enum LineType {
        LINES,
        TUBES,
        RIBBONS
    };

    Streamlines();
    virtual ~Streamlines();

    virtual const char *getClassName() const 
    {
        return "Streamlines";
    }

    virtual void setLighting(bool state);

    virtual void setOpacity(double opacity);

    virtual void setVisibility(bool state);

    virtual bool getVisibility();

    virtual void setEdgeVisibility(bool state);

    virtual void setEdgeColor(float color[3]);

    virtual void setEdgeWidth(float edgeWidth);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

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

    void setSeedVisibility(bool state);

    void setSeedColor(float color[3]);

private:
    virtual void initProp();
    virtual void update();

    static void getRandomPoint(double pt[3], const double bounds[6]);
    static void getRandomCellPt(vtkDataSet *ds, double pt[3]);

    LineType _lineType;
    float _seedColor[3];
    bool _seedVisible;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkActor> _linesActor;
    vtkSmartPointer<vtkActor> _seedActor;
    vtkSmartPointer<vtkStreamTracer> _streamTracer;
    vtkSmartPointer<vtkPolyDataAlgorithm> _lineFilter;
    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
    vtkSmartPointer<vtkPolyDataMapper> _seedMapper;
};

}
}

#endif
