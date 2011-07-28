/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_RENDERER_H__
#define __RAPPTURE_VTKVIS_RENDERER_H__

#include <vtkSmartPointer.h>
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
#include "RpContour2D.h"
#include "RpContour3D.h"
#include "RpGlyphs.h"
#include "RpHeightMap.h"
#include "RpLIC.h"
#include "RpMolecule.h"
#include "RpPolyData.h"
#include "RpPseudoColor.h"
#include "RpStreamlines.h"
#include "RpVolume.h"
#include "Trace.h"

// Controls if TGA format is sent to client
//#define RENDER_TARGA
#define TARGA_BYTES_PER_PIXEL 3

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

    enum AxesFlyMode {
        FLY_OUTER_EDGES = 0,
        FLY_CLOSEST_TRIAD,
        FLY_FURTHEST_TRIAD,
        FLY_STATIC_EDGES,
        FLY_STATIC_TRIAD
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
    typedef std::tr1::unordered_map<DataSetId, Contour2D *> Contour2DHashmap;
    typedef std::tr1::unordered_map<DataSetId, Contour3D *> Contour3DHashmap;
    typedef std::tr1::unordered_map<DataSetId, Glyphs *> GlyphsHashmap;
    typedef std::tr1::unordered_map<DataSetId, HeightMap *> HeightMapHashmap;
    typedef std::tr1::unordered_map<DataSetId, LIC *> LICHashmap;
    typedef std::tr1::unordered_map<DataSetId, Molecule *> MoleculeHashmap;
    typedef std::tr1::unordered_map<DataSetId, PolyData *> PolyDataHashmap;
    typedef std::tr1::unordered_map<DataSetId, PseudoColor *> PseudoColorHashmap;
    typedef std::tr1::unordered_map<DataSetId, Streamlines *> StreamlinesHashmap;
    typedef std::tr1::unordered_map<DataSetId, Volume *> VolumeHashmap;

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

    void setUseCumulativeDataRange(bool state, bool onlyVisible = false);

    // Render window

    vtkRenderWindow *getRenderWindow()
    {
        return _renderWindow;
    }

    void setWindowSize(int width, int height);

    int getWindowWidth() const;

    int getWindowHeight() const;

    // Camera controls

    void setViewAngle(int height);

    void setCameraMode(CameraMode mode);

    CameraMode getCameraMode() const;

    void resetCamera(bool resetOrientation = true);

    void setCameraZoomRegion(double x, double y, double width, double height);

    void getCameraZoomRegion(double xywh[4]) const;

    void getScreenWorldCoords(double xywh[4]) const;

    void rotateCamera(double yaw, double pitch, double roll);

    void setCameraOrientation(double quat[4]);

    void setCameraOrientationAndPosition(double position[3],
                                         double focalPoint[3],
                                         double viewUp[3]);

    void getCameraOrientationAndPosition(double position[3],
                                         double focalPoint[3],
                                         double viewUp[3]);

    void panCamera(double x, double y, bool absolute = true);

    void zoomCamera(double z, bool absolute = true);

    // Rendering an image

    void setBackgroundColor(float color[3]);

    void showBounds(bool state);

    void setClipPlane(Axis axis, double ratio, int direction);

    void setUseDepthPeeling(bool state);

    bool render();

    void getRenderedFrame(vtkUnsignedCharArray *imgData);

    // Axes

    void setAxesFlyMode(AxesFlyMode mode);

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

    bool renderColorMap(const ColorMapId& id, 
                        const DataSetId& dataSetID,
                        const char *title,
                        int width, int height,
                        vtkUnsignedCharArray *imgData);

    // 2D Contour plots

    void addContour2D(const DataSetId& id);

    void deleteContour2D(const DataSetId& id);

    Contour2D *getContour2D(const DataSetId& id);

    void setContour2DContours(const DataSetId& id, int numContours);

    void setContour2DContourList(const DataSetId& id, const std::vector<double>& contours);

    void setContour2DOpacity(const DataSetId& id, double opacity);

    void setContour2DVisibility(const DataSetId& id, bool state);

    void setContour2DEdgeColor(const DataSetId& id, float color[3]);

    void setContour2DEdgeWidth(const DataSetId& id, float edgeWidth);

    void setContour2DLighting(const DataSetId& id, bool state);

    // 3D Contour (isosurface) plots

    void addContour3D(const DataSetId& id);

    void deleteContour3D(const DataSetId& id);

    Contour3D *getContour3D(const DataSetId& id);

    void setContour3DContours(const DataSetId& id, int numContours);

    void setContour3DContourList(const DataSetId& id, const std::vector<double>& contours);

    void setContour3DColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setContour3DOpacity(const DataSetId& id, double opacity);

    void setContour3DVisibility(const DataSetId& id, bool state);

    void setContour3DColor(const DataSetId& id, float color[3]);

    void setContour3DEdgeVisibility(const DataSetId& id, bool state);

    void setContour3DEdgeColor(const DataSetId& id, float color[3]);

    void setContour3DEdgeWidth(const DataSetId& id, float edgeWidth);

    void setContour3DWireframe(const DataSetId& id, bool state);

    void setContour3DLighting(const DataSetId& id, bool state);

    // Glyphs

    void addGlyphs(const DataSetId& id);

    void deleteGlyphs(const DataSetId& id);

    Glyphs *getGlyphs(const DataSetId& id);

    void setGlyphsColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setGlyphsShape(const DataSetId& id, Glyphs::GlyphShape shape);

    void setGlyphsScaleFactor(const DataSetId& id, double scale);

    void setGlyphsOpacity(const DataSetId& id, double opacity);

    void setGlyphsVisibility(const DataSetId& id, bool state);

    void setGlyphsLighting(const DataSetId& id, bool state);

    // Height maps

    void addHeightMap(const DataSetId& id);

    void deleteHeightMap(const DataSetId& id);

    HeightMap *getHeightMap(const DataSetId& id);

    void setHeightMapVolumeSlice(const DataSetId& id, HeightMap::Axis axis, double ratio);

    void setHeightMapHeightScale(const DataSetId& id, double scale);

    void setHeightMapColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setHeightMapContours(const DataSetId& id, int numContours);

    void setHeightMapContourList(const DataSetId& id, const std::vector<double>& contours);

    void setHeightMapOpacity(const DataSetId& id, double opacity);

    void setHeightMapVisibility(const DataSetId& id, bool state);

    void setHeightMapEdgeVisibility(const DataSetId& id, bool state);

    void setHeightMapEdgeColor(const DataSetId& id, float color[3]);

    void setHeightMapEdgeWidth(const DataSetId& id, float edgeWidth);

    void setHeightMapContourVisibility(const DataSetId& id, bool state);

    void setHeightMapContourEdgeColor(const DataSetId& id, float color[3]);

    void setHeightMapContourEdgeWidth(const DataSetId& id, float edgeWidth);

    void setHeightMapLighting(const DataSetId& id, bool state);

    // LIC plots

    void addLIC(const DataSetId& id);
    
    void deleteLIC(const DataSetId& id);

    LIC *getLIC(const DataSetId& id);

    void setLICVolumeSlice(const DataSetId& id, LIC::Axis axis, double ratio);

    void setLICColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setLICOpacity(const DataSetId& id, double opacity);

    void setLICVisibility(const DataSetId& id, bool state);

    void setLICEdgeVisibility(const DataSetId& id, bool state);

    void setLICEdgeColor(const DataSetId& id, float color[3]);

    void setLICEdgeWidth(const DataSetId& id, float edgeWidth);

    void setLICLighting(const DataSetId& id, bool state);

    // Molecules

    void addMolecule(const DataSetId& id);
    
    void deleteMolecule(const DataSetId& id);

    Molecule *getMolecule(const DataSetId& id);

    void setMoleculeColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setMoleculeOpacity(const DataSetId& id, double opacity);

    void setMoleculeAtomScaling(const DataSetId& id, Molecule::AtomScaling scaling);

    void setMoleculeAtomVisibility(const DataSetId& id, bool state);

    void setMoleculeBondVisibility(const DataSetId& id, bool state);

    void setMoleculeVisibility(const DataSetId& id, bool state);

    void setMoleculeEdgeVisibility(const DataSetId& id, bool state);

    void setMoleculeEdgeColor(const DataSetId& id, float color[3]);

    void setMoleculeEdgeWidth(const DataSetId& id, float edgeWidth);

    void setMoleculeWireframe(const DataSetId& id, bool state);

    void setMoleculeLighting(const DataSetId& id, bool state);

    // PolyData Meshes

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

    // Color-mapped surfaces

    void addPseudoColor(const DataSetId& id);

    void deletePseudoColor(const DataSetId& id);

    PseudoColor *getPseudoColor(const DataSetId& id);

    void setPseudoColorColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setPseudoColorOpacity(const DataSetId& id, double opacity);

    void setPseudoColorVisibility(const DataSetId& id, bool state);

    void setPseudoColorEdgeVisibility(const DataSetId& id, bool state);

    void setPseudoColorEdgeColor(const DataSetId& id, float color[3]);

    void setPseudoColorEdgeWidth(const DataSetId& id, float edgeWidth);

    void setPseudoColorWireframe(const DataSetId& id, bool state);

    void setPseudoColorLighting(const DataSetId& id, bool state);

    // Streamlines

    void addStreamlines(const DataSetId& id);

    void deleteStreamlines(const DataSetId& id);

    Streamlines *getStreamlines(const DataSetId& id);

    void setStreamlinesSeedToRandomPoints(const DataSetId& id, int numPoints);

    void setStreamlinesSeedToRake(const DataSetId& id, double start[3], double end[3], int numPoints);

    void setStreamlinesSeedToPolygon(const DataSetId& id, double center[3], double normal[3],
                                           double radius, int numSides);

    void setStreamlinesLength(const DataSetId& id, double length);

    void setStreamlinesTypeToLines(const DataSetId& id);

    void setStreamlinesTypeToTubes(const DataSetId& id, int numSides, double radius);

    void setStreamlinesTypeToRibbons(const DataSetId& id, double width, double angle);

    void setStreamlinesOpacity(const DataSetId& id, double opacity);

    void setStreamlinesVisibility(const DataSetId& id, bool state);

    void setStreamlinesSeedVisibility(const DataSetId& id, bool state);

    void setStreamlinesColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setStreamlinesSeedColor(const DataSetId& id, float color[3]);

    void setStreamlinesEdgeVisibility(const DataSetId& id, bool state);

    void setStreamlinesEdgeColor(const DataSetId& id, float color[3]);

    void setStreamlinesEdgeWidth(const DataSetId& id, float edgeWidth);

    void setStreamlinesLighting(const DataSetId& id, bool state);

    // Volumes

    void addVolume(const DataSetId& id);

    void deleteVolume(const DataSetId& id);

    Volume *getVolume(const DataSetId& id);

    void setVolumeColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setVolumeOpacity(const DataSetId& id, double opacity);

    void setVolumeVisibility(const DataSetId& id, bool state);

    void setVolumeAmbient(const DataSetId& id, double coeff);

    void setVolumeDiffuse(const DataSetId& id, double coeff);

    void setVolumeSpecular(const DataSetId& id, double coeff, double power);

    void setVolumeLighting(const DataSetId& id, bool state);

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

    void collectDataRanges(double *range, bool onlyVisible);

    void updateRanges(bool useCumulative);

    void computeDisplayToWorld(double x, double y, double z, double worldPt[4]);

    void computeWorldToDisplay(double x, double y, double z, double displayPt[3]);

    void computeScreenWorldCoords();

    void storeCameraOrientation();
    void restoreCameraOrientation();
    void initCamera();
    void initAxes();
    void resetAxes();
    void setCameraClippingPlanes();

    bool _needsRedraw;
    int _windowWidth, _windowHeight;
    double _imgWorldOrigin[2];
    double _imgWorldDims[2];
    double _screenWorldCoords[4];
    double _cameraPos[3];
    double _cameraFocalPoint[3];
    double _cameraUp[3];
    double _cameraZoomRatio;
    double _cameraPan[2];
    float _bgColor[3];
    bool _useCumulativeRange;
    bool _cumulativeRangeOnlyVisible;
    double _cumulativeDataRange[2];

    ColorMapHashmap _colorMaps;
    DataSetHashmap _dataSets;
    Contour2DHashmap _contour2Ds;
    Contour3DHashmap _contour3Ds;
    GlyphsHashmap _glyphs;
    HeightMapHashmap _heightMaps;
    LICHashmap _lics;
    MoleculeHashmap _molecules;
    PolyDataHashmap _polyDatas;
    PseudoColorHashmap _pseudoColors;
    StreamlinesHashmap _streamlines;
    VolumeHashmap _volumes;

    CameraMode _cameraMode;

    vtkSmartPointer<vtkPlane> _cameraClipPlanes[4];
    vtkSmartPointer<vtkPlane> _userClipPlanes[6];
    vtkSmartPointer<vtkPlaneCollection> _activeClipPlanes;
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
