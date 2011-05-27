/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_HEIGHTMAP_H__
#define __RAPPTURE_VTKVIS_HEIGHTMAP_H__

#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkWarpScalar.h>
#include <vtkPropAssembly.h>
#include <vtkPolyData.h>

#include <vector>

#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Color-mapped plot of data set
 */
class HeightMap {
public:
    enum Axis {
        X_AXIS,
        Y_AXIS,
        Z_AXIS
    };

    HeightMap();
    virtual ~HeightMap();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void selectVolumeSlice(Axis axis, double ratio);

    void setHeightScale(double scale);

    void setContours(int numContours);

    void setContours(int numContours, double range[2]);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setVisibility(bool state);

    bool getVisibility();

    void setOpacity(double opacity);

    void setEdgeVisibility(bool state);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setContourVisibility(bool state);

    void setContourEdgeColor(float color[3]);

    void setContourEdgeWidth(float edgeWidth);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    vtkPolyData *initWarp(vtkPolyData *input);
    void initProp();
    void update();

    DataSet * _dataSet;

    int _numContours;
    std::vector<double> _contours;
    double _dataRange[2];

    float _edgeColor[3];
    float _contourEdgeColor[3];
    float _edgeWidth;
    float _contourEdgeWidth;
    double _opacity;
    double _warpScale;

    vtkSmartPointer<vtkPolyData> _transformedData;
    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
    vtkSmartPointer<vtkGaussianSplatter> _pointSplatter;
    vtkSmartPointer<vtkExtractVOI> _volumeSlicer;
    vtkSmartPointer<vtkWarpScalar> _warp;
    vtkSmartPointer<vtkActor> _dsActor;
    vtkSmartPointer<vtkActor> _contourActor;
    vtkSmartPointer<vtkPropAssembly> _props;
};

}
}

#endif
