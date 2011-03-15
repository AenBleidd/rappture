/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_RENDERER_H__
#define __RAPPTURE_VTKVIS_RENDERER_H__

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkCubeAxesActor.h>
#ifdef USE_CUSTOM_AXES
#include <vtkRpCubeAxesActor2D.h>
#else
#include <vtkCubeAxesActor2D.h>
#endif
#include <vtkScalarBarActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>

#include <string>
#include <vector>
#include <tr1/unordered_map>

#include "ColorMap.h"
#include "RpVtkDataSet.h"
#include "RpPseudoColor.h"
#include "RpContour2D.h"
#include "RpPolyData.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK Renderer
 */
class Renderer
{
public:
    Renderer();
    virtual ~Renderer();

    enum Axis {
        X_AXIS,
        Y_AXIS,
        Z_AXIS
    };

    enum CameraMode {
        PERSPECTIVE,
        ORTHO,
        IMAGE
    };

    typedef std::string DataSetId;
    typedef std::string ColorMapId;
    typedef std::tr1::unordered_map<DataSetId, DataSet *> DataSetHashmap;
    typedef std::tr1::unordered_map<ColorMapId, ColorMap *> ColorMapHashmap;
    typedef std::tr1::unordered_map<DataSetId, PseudoColor *> PseudoColorHashmap;
    typedef std::tr1::unordered_map<DataSetId, Contour2D *> Contour2DHashmap;
    typedef std::tr1::unordered_map<DataSetId, PolyData *> PolyDataHashmap;

    // Data sets

    void addDataSet(const DataSetId& id);

    void deleteDataSet(const DataSetId& id);

    DataSet *getDataSet(const DataSetId& id);

    bool setData(const DataSetId& id, char *data, int nbytes);

    bool setDataFile(const DataSetId& id, const char *filename);

    double getDataValueAtPixel(const DataSetId& id, int x, int y);

    double getDataValue(const DataSetId& id, double x, double y, double z);

    void setOpacity(const DataSetId& id, double opacity);

    void setVisibility(const DataSetId& id, bool state);

    // Render window

    void setWindowSize(int width, int height);

    int getWindowWidth() const;

    int getWindowHeight() const;

    // Camera controls

    void setCameraMode(CameraMode mode);

    void resetCamera(bool resetOrientation = true);

    void setCameraZoomRegion(double x, double y, double width, double height);

    void rotateCamera(double yaw, double pitch, double roll);

    void panCamera(double x, double y);

    void zoomCamera(double z);

    // Rendering an image

    void setBackgroundColor(float color[3]);

    bool render();

    void getRenderedFrame(vtkUnsignedCharArray *imgData);

    // Axes

    void setAxesGridVisibility(bool state);

    void setAxesVisibility(bool state);

    void setAxesColor(double color[3]);

    void setAxisGridVisibility(Axis axis, bool state);

    void setAxisVisibility(Axis axis, bool state);

    void setAxisTitle(Axis axis, const char *title);

    void setAxisUnits(Axis axis, const char *units);

    // Colormaps

    void addColorMap(const ColorMapId& id, ColorMap *colorMap);

    void deleteColorMap(const ColorMapId& id);

    ColorMap *getColorMap(const ColorMapId& id);

    bool renderColorMap(const ColorMapId& id, const char *title,
                        int width, int height,
                        vtkUnsignedCharArray *imgData);

    // Color-mapped surfaces

    void addPseudoColor(const DataSetId& id);

    void deletePseudoColor(const DataSetId& id);

    PseudoColor *getPseudoColor(const DataSetId& id);

    void setPseudoColorColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    vtkLookupTable *getPseudoColorColorMap(const DataSetId& id);

    void setPseudoColorOpacity(const DataSetId& id, double opacity);

    void setPseudoColorVisibility(const DataSetId& id, bool state);

    void setPseudoColorEdgeVisibility(const DataSetId& id, bool state);

    void setPseudoColorEdgeColor(const DataSetId& id, float color[3]);

    void setPseudoColorEdgeWidth(const DataSetId& id, float edgeWidth);

    void setPseudoColorLighting(const DataSetId& id, bool state);

    // Contour plots

    void addContour2D(const DataSetId& id);

    void deleteContour2D(const DataSetId& id);

    Contour2D *getContour2D(const DataSetId& id);

    void setContours(const DataSetId& id, int numContours);

    void setContourList(const DataSetId& id, const std::vector<double>& contours);

    void setContourOpacity(const DataSetId& id, double opacity);

    void setContourVisibility(const DataSetId& id, bool state);

    void setContourEdgeColor(const DataSetId& id, float color[3]);

    void setContourEdgeWidth(const DataSetId& id, float edgeWidth);

    void setContourLighting(const DataSetId& id, bool state);

    // Meshes

    void addPolyData(const DataSetId& id);
    
    void deletePolyData(const DataSetId& id);

    PolyData *getPolyData(const DataSetId& id);

    void setPolyDataOpacity(const DataSetId& id, double opacity);

    void setPolyDataVisibility(const DataSetId& id, bool state);

    void setPolyDataColor(const DataSetId& id, float color[3]);

    void setPolyDataEdgeVisibility(const DataSetId& id, bool state);

    void setPolyDataEdgeColor(const DataSetId& id, float color[3]);

    void setPolyDataEdgeWidth(const DataSetId& id, float edgeWidth);

    void setPolyDataWireframe(const DataSetId& id, bool state);

    void setPolyDataLighting(const DataSetId& id, bool state);

private:
    static void printCameraInfo(vtkCamera *camera);
    static inline double min2(double a, double b)
    {
        return ((a < b) ? a : b);
    }
    static inline double max2(double a, double b)
    {
        return ((a > b) ? a : b);
    }
    static void mergeBounds(double *boundsDest, const double *bounds1, const double *bounds2);

    void collectBounds(double *bounds, bool onlyVisible);

    void collectDataRanges(double *range);

    void storeCameraOrientation();
    void restoreCameraOrientation();
    void initCamera();
    void initAxes();
    void resetAxes();

    bool _needsRedraw;
    int _windowWidth, _windowHeight;
    double _imgWorldOrigin[2];
    double _imgWorldDims[2];
    double _cameraPos[3];
    double _cameraFocalPoint[3];
    double _cameraUp[3];
    float _bgColor[3];
    double _cumulativeDataRange[2];

    ColorMapHashmap _colorMaps;
    DataSetHashmap _dataSets;
    PseudoColorHashmap _pseudoColors;
    Contour2DHashmap _contours;
    PolyDataHashmap _polyDatas;

    CameraMode _cameraMode;

    vtkSmartPointer<vtkPlaneCollection> _clippingPlanes;
    vtkSmartPointer<vtkCubeAxesActor> _cubeAxesActor; // For 3D view
#ifdef USE_CUSTOM_AXES
    vtkSmartPointer<vtkRpCubeAxesActor2D> _cubeAxesActor2D; // For 2D view
#else
    vtkSmartPointer<vtkCubeAxesActor2D> _cubeAxesActor2D; // For 2D view
#endif
    vtkSmartPointer<vtkScalarBarActor> _scalarBarActor;
    vtkSmartPointer<vtkRenderer> _renderer;
    vtkSmartPointer<vtkRenderer> _legendRenderer;
    vtkSmartPointer<vtkRenderWindow> _renderWindow;
    vtkSmartPointer<vtkRenderWindow> _legendRenderWindow;
};

}
}

#endif
