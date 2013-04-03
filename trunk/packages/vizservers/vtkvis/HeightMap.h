/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_HEIGHTMAP_H
#define VTKVIS_HEIGHTMAP_H

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
#include <vtkPolyDataNormals.h>
#include <vtkAssembly.h>
#include <vtkPolyData.h>
#include <vtkPlane.h>

#include <vector>

#include "ColorMap.h"
#include "Types.h"
#include "VtkGraphicsObject.h"

namespace VtkVis {

/**
 * \brief Color-mapped plot of data set
 */
class HeightMap : public VtkGraphicsObject {
public:
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };

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

    virtual void setColor(float color[3]);

    virtual void setEdgeVisibility(bool state);

    virtual void setEdgeColor(float color[3]);

    virtual void setEdgeWidth(float edgeWidth);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    virtual void setAspect(double aspect);

    void selectVolumeSlice(Axis axis, double ratio);

    void setHeightScale(double scale);

    double getHeightScale()
    {
        return _warpScale;
    }

    void setInterpolateBeforeMapping(bool state);

    void setNumContours(int numContours);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setContourLineColorMapEnabled(bool mode);

    void setColorMode(ColorMode mode, DataSet::DataAttributeType type,
                      const char *name, double range[2] = NULL);

    void setColorMode(ColorMode mode,
                      const char *name, double range[2] = NULL);

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

    virtual void updateRanges(Renderer *renderer);

    void setContourLineVisibility(bool state);

    void setContourSurfaceVisibility(bool state);

    void setContourEdgeColor(float color[3]);

    void setContourEdgeWidth(float edgeWidth);

private:
    HeightMap();

    virtual void initProp();
    virtual void update();

    void computeDataScale();

    vtkAlgorithmOutput *initWarp(vtkAlgorithmOutput *input);
    vtkAlgorithmOutput *initWarp(vtkPolyData *input);

    int _numContours;
    std::vector<double> _contours;

    bool _contourColorMap;
    float _contourEdgeColor[3];
    float _contourEdgeWidth;
    double _warpScale;
    double _dataScale;
    Axis _sliceAxis;
    bool _pipelineInitialized;

    ColorMap *_colorMap;
    ColorMode _colorMode;
    std::string _colorFieldName;
    DataSet::DataAttributeType _colorFieldType;
    double _colorFieldRange[2];
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    Renderer *_renderer;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkDataSetMapper> _dsMapper;
    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
    vtkSmartPointer<vtkGaussianSplatter> _pointSplatter;
    vtkSmartPointer<vtkExtractVOI> _volumeSlicer;
    vtkSmartPointer<vtkPlane> _cutPlane;
    vtkSmartPointer<vtkWarpScalar> _warp;
    vtkSmartPointer<vtkPolyDataNormals> _normalsGenerator;
    vtkSmartPointer<vtkActor> _dsActor;
    vtkSmartPointer<vtkActor> _contourActor;
};

}

#endif
