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

#include "RpVtkDataSet.h"
#include "RpPseudoColor.h"
#include "RpContour2D.h"

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

    typedef std::string DataSetId;
    typedef std::string ColorMapId;
    typedef std::tr1::unordered_map<DataSetId, DataSet *> DataSetHashmap;
    typedef std::tr1::unordered_map<ColorMapId, vtkSmartPointer<vtkLookupTable> > ColorMapHashmap;
    typedef std::tr1::unordered_map<DataSetId, PseudoColor *> PseudoColorHashmap;
    typedef std::tr1::unordered_map<DataSetId, Contour2D *> Contour2DHashmap;

    void addDataSet(DataSetId id);

    void deleteDataSet(DataSetId id);

    DataSet *getDataSet(DataSetId id);

    bool setData(DataSetId id, char *data, int nbytes);

    bool setDataFile(DataSetId id, const char *filename);

    double getDataValueAtPixel(DataSetId id, int x, int y);

    double getDataValue(DataSetId id, double x, double y, double z);

    void setWindowSize(int width, int height);

    void setZoomRegion(double x, double y, double width, double height);

    void setBackgroundColor(float color[3]);

    bool render();

    int getWindowWidth() const;

    int getWindowHeight() const;

    void getRenderedFrame(vtkUnsignedCharArray *imgData);

    //

    void setAxesVisibility(bool state);

    void setAxisVisibility(Axis axis, bool state);

    //

    void addColorMap(ColorMapId id, vtkLookupTable *lut);

    void deleteColorMap(ColorMapId id);

    vtkLookupTable *getColorMap(ColorMapId id);

    void renderColorMap(ColorMapId id, const char *title,
                        int width, int height,
                        vtkUnsignedCharArray *imgData);

    void setOpacity(DataSetId id, double opacity);

    void setVisibility(DataSetId id, bool state);

    //

    void addPseudoColor(DataSetId id);

    void deletePseudoColor(DataSetId id);

    PseudoColor *getPseudoColor(DataSetId id);

    void setPseudoColorColorMap(DataSetId id, ColorMapId colorMapId);

    vtkLookupTable *getPseudoColorColorMap(DataSetId id);

    void setPseudoColorVisibility(DataSetId id, bool state);

    //

    void addContour2D(DataSetId id);

    void deleteContour2D(DataSetId id);

    Contour2D *getContour2D(DataSetId id);

    void setContours(DataSetId id, int numContours);

    void setContourList(DataSetId id, const std::vector<double>& contours);

    void setContourVisibility(DataSetId id, bool state);

    void setContourEdgeColor(DataSetId id, float color[3]);

    void setContourEdgeWidth(DataSetId id, float edgeWidth);

private:
    static void printCameraInfo(vtkCamera *camera);

    void initCamera(double bounds[6]);
    void initAxes();

    bool _needsRedraw;
    int _windowWidth, _windowHeight;
    double _imgWorldOrigin[2];
    double _imgWorldDims[2];
    float _bgColor[3];

    ColorMapHashmap _colorMaps;
    DataSetHashmap _dataSets;
    PseudoColorHashmap _pseudoColors;
    Contour2DHashmap _contours;

    vtkSmartPointer<vtkPlaneCollection> _clippingPlanes;
#ifdef USE_CUSTOM_AXES
    vtkSmartPointer<vtkRpCubeAxesActor2D> _axesActor;
#else
    vtkSmartPointer<vtkCubeAxesActor2D> _axesActor;
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
