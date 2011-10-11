/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_HEIGHTMAP_H__
#define __RAPPTURE_VTKVIS_HEIGHTMAP_H__

#include <vtkSmartPointer.h>
#include <vtkAlgorithmOutput.h>
#include <vtkContourFilter.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkWarpScalar.h>
#include <vtkAssembly.h>
#include <vtkPolyData.h>
#include <vtkPlane.h>

#include <vector>

#include "ColorMap.h"
#include "RpTypes.h"
#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Color-mapped plot of data set
 */
class HeightMap : public VtkGraphicsObject {
public:
    HeightMap(int numContours, double heightScale = 1.0);

    HeightMap(const std::vector<double>& contours, double heightScale = 1.0);

    virtual ~HeightMap();

    virtual const char *getClassName() const
    {
        return "HeightMap";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setLighting(bool state);

    virtual void setEdgeVisibility(bool state);

    virtual void setEdgeColor(float color[3]);

    virtual void setEdgeWidth(float edgeWidth);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void selectVolumeSlice(Axis axis, double ratio);

    void setHeightScale(double scale);

    double getHeightScale()
    {
        return _warpScale;
    }

    void setNumContours(int numContours);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setColorMap(ColorMap *colorMap);

    /**
     * \brief Return the ColorMap in use
     */
    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void updateColorMap();

    virtual void updateRanges(Renderer *renderer);

    void setContourLineVisibility(bool state);

    void setContourSurfaceVisibility(bool state);

    void setContourEdgeColor(float color[3]);

    void setContourEdgeWidth(float edgeWidth);

private:
    HeightMap();

    virtual void initProp();
    virtual void update();

    vtkAlgorithmOutput *initWarp(vtkAlgorithmOutput *input);
    vtkAlgorithmOutput *initWarp(vtkPolyData *input);

    int _numContours;
    std::vector<double> _contours;
    ColorMap *_colorMap;

    float _contourEdgeColor[3];
    float _contourEdgeWidth;
    double _warpScale;
    double _dataScale;
    Axis _sliceAxis;
    bool _pipelineInitialized;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
    vtkSmartPointer<vtkGaussianSplatter> _pointSplatter;
    vtkSmartPointer<vtkExtractVOI> _volumeSlicer;
    vtkSmartPointer<vtkPlane> _cutPlane;
    vtkSmartPointer<vtkWarpScalar> _warp;
    vtkSmartPointer<vtkActor> _dsActor;
    vtkSmartPointer<vtkActor> _contourActor;
};

}
}

#endif
