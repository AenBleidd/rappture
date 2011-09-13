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

#include "ColorMap.h"
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
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };
    enum StepUnit {
        LENGTH_UNIT,
        CELL_LENGTH_UNIT
    };
    enum IntegratorType {
        RUNGE_KUTTA2,
        RUNGE_KUTTA4,
        RUNGE_KUTTA45
    };
    enum IntegrationDirection {
        FORWARD,
        BACKWARD,
        BOTH
    };

    Streamlines();
    virtual ~Streamlines();

    virtual const char *getClassName() const 
    {
        return "Streamlines";
    }

    virtual void setDataSet(DataSet *dataSet,
                            bool useCumulative,
                            double scalarRange[2],
                            double vectorMagnitudeRange[2],
                            double vectorComponentRange[3][2]);

    virtual void setLighting(bool state);

    virtual void setOpacity(double opacity);

    virtual void setVisibility(bool state);

    virtual bool getVisibility();

    virtual void setColor(float color[3]);

    virtual void setEdgeVisibility(bool state);

    virtual void setEdgeColor(float color[3]);

    virtual void setEdgeWidth(float edgeWidth);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setSeedToMeshPoints();

    void setSeedToFilledMesh(int numPoints);

    void setSeedToMeshPoints(vtkDataSet *ds);

    void setSeedToFilledMesh(vtkDataSet *ds, int numPoints);

    void setSeedToRake(double start[3], double end[3], int numPoints);

    void setSeedToDisk(double center[3], double normal[3],
                       double radius, double innerRadius, int numPoints);

    void setSeedToPolygon(double center[3], double normal[3],
                          double angle, double radius,
                          int numSides);

    void setSeedToFilledPolygon(double center[3], double normal[3],
                                double angle, double radius,
                                int numSides, int numPoints);

    void setIntegrator(IntegratorType integrator);

    void setIntegrationDirection(IntegrationDirection dir);

    void setIntegrationStepUnit(StepUnit unit);

    void setInitialIntegrationStep(double step);

    void setMinimumIntegrationStep(double step);

    void setMaximumIntegrationStep(double step);

    void setMaximumError(double error);

    void setMaxPropagation(double length);

    void setMaxNumberOfSteps(int steps);

    void setTerminalSpeed(double speed);

    void setLineTypeToLines();

    void setLineTypeToTubes(int numSides, double radius);

    void setLineTypeToRibbons(double width, double angle);

    void setColorMode(ColorMode mode);

    void setColorMap(ColorMap *colorMap);

    /**
     * \brief Return the ColorMap in use
     */
    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void updateColorMap();

    virtual void updateRanges(bool useCumulative,
                              double scalarRange[2],
                              double vectorMagnitudeRange[2],
                              double vectorComponentRange[3][2]);

    void setSeedVisibility(bool state);

    void setSeedColor(float color[3]);

private:
    virtual void initProp();
    virtual void update();

    static double getRandomNum(double min, double max);
    static void getRandomPoint(double pt[3], const double bounds[6]);
    static void getRandomPointInTriangle(double pt[3],
                                         const double v0[3],
                                         const double v1[3],
                                         const double v2[3]);
    static void getRandomPointOnLineSegment(double pt[3],
                                            const double endpt[3],
                                            const double endpt2[3]);
    static void getRandomPointInTetrahedron(double pt[3],
                                            const double v0[3],
                                            const double v1[3],
                                            const double v2[3],
                                            const double v3[3]);
    static void getRandomCellPt(double pt[3], vtkDataSet *ds);

    LineType _lineType;
    ColorMode _colorMode;
    ColorMap *_colorMap;
    float _seedColor[3];
    bool _seedVisible;
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    double _dataScale;

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
