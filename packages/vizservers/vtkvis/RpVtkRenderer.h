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
#include "vtkRpCubeAxesActor2D.h"
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
#include "RpCutplane.h"
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

    enum PrincipalPlane {
        PLANE_XY,
        PLANE_ZY,
        PLANE_XZ
    };

    enum AxesFlyMode {
        FLY_OUTER_EDGES = 0,
        FLY_CLOSEST_TRIAD,
        FLY_FURTHEST_TRIAD,
        FLY_STATIC_EDGES,
        FLY_STATIC_TRIAD
    };

    enum AxesTickPosition {
        TICKS_INSIDE,
        TICKS_OUTSIDE,
        TICKS_BOTH
    };

    enum CameraMode {
        PERSPECTIVE,
        ORTHO,
        IMAGE
    };

    enum LegendType {
        ACTIVE_SCALAR,
        ACTIVE_VECTOR_MAGNITUDE,
        ACTIVE_VECTOR_X,
        ACTIVE_VECTOR_Y,
        ACTIVE_VECTOR_Z
    };

    typedef std::string DataSetId;
    typedef std::string ColorMapId;
    typedef std::tr1::unordered_map<DataSetId, DataSet *> DataSetHashmap;
    typedef std::tr1::unordered_map<ColorMapId, ColorMap *> ColorMapHashmap;
    typedef std::tr1::unordered_map<DataSetId, Contour2D *> Contour2DHashmap;
    typedef std::tr1::unordered_map<DataSetId, Contour3D *> Contour3DHashmap;
    typedef std::tr1::unordered_map<DataSetId, Cutplane *> CutplaneHashmap;
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

    void getDataSetNames(std::vector<std::string>& names);

    bool setData(const DataSetId& id, char *data, int nbytes);

    bool setDataFile(const DataSetId& id, const char *filename);

    bool setDataSetActiveScalars(const DataSetId& id, const char *scalarName);

    bool setDataSetActiveVectors(const DataSetId& id, const char *vectorName);

    bool getScalarValueAtPixel(const DataSetId& id, int x, int y, double *value);

    bool getScalarValue(const DataSetId& id, double x, double y, double z, double *value);

    bool getVectorValueAtPixel(const DataSetId& id, int x, int y, double vector[3]);

    bool getVectorValue(const DataSetId& id, double x, double y, double z, double vector[3]);

    void setDataSetShowBounds(const DataSetId& id, bool state);

    void setDataSetOutlineColor(const DataSetId& id, float color[3]);

    void setDataSetOpacity(const DataSetId& id, double opacity);

    void setDataSetVisibility(const DataSetId& id, bool state);

    void setUseCumulativeDataRange(bool state, bool onlyVisible = false);

    // Render window

    /// Get the VTK render window object this Renderer uses
    vtkRenderWindow *getRenderWindow()
    {
        return _renderWindow;
    }

    void setWindowSize(int width, int height);

    int getWindowWidth() const;

    int getWindowHeight() const;

    // Camera controls

    void setViewAngle(int height);

    vtkCamera *getVtkCamera()
    {
        if (_renderer != NULL)
            return _renderer->GetActiveCamera();
        else
            return NULL;
    }

    void setCameraMode(CameraMode mode);

    CameraMode getCameraMode() const;

    void resetCamera(bool resetOrientation = true);

    void resetCameraClippingRange();

    void setCameraZoomRegionPixels(int x, int y, int width, int height);

    void setCameraZoomRegion(double x, double y, double width, double height);

    void getCameraZoomRegion(double xywh[4]) const;

    void getScreenWorldCoords(double xywh[4]) const;

    void rotateCamera(double yaw, double pitch, double roll);

    void setCameraOrientation(const double quat[4], bool absolute = true);

    void panCamera(double x, double y, bool absolute = true);

    void zoomCamera(double z, bool absolute = true);

    void setCameraOrientationAndPosition(const double position[3],
                                         const double focalPoint[3],
                                         const double viewUp[3]);

    void getCameraOrientationAndPosition(double position[3],
                                         double focalPoint[3],
                                         double viewUp[3]);

    // Rendering an image

    void setBackgroundColor(float color[3]);

    void setClipPlane(Axis axis, double ratio, int direction);

    void setUseTwoSidedLighting(bool state);

    void setUseDepthPeeling(bool state);

    void eventuallyRender();

    bool render();

    void getRenderedFrame(vtkUnsignedCharArray *imgData);

    // Axes

    void setAxesFlyMode(AxesFlyMode mode);

    void setAxesVisibility(bool state);

    void setAxesGridVisibility(bool state);

    void setAxesLabelVisibility(bool state);

    void setAxesTickVisibility(bool state);

    void setAxesTickPosition(AxesTickPosition pos);

    void setAxesColor(double color[3]);

    void setAxisVisibility(Axis axis, bool state);

    void setAxisGridVisibility(Axis axis, bool state);

    void setAxisLabelVisibility(Axis axis, bool state);

    void setAxisTickVisibility(Axis axis, bool state);

    void setAxisTitle(Axis axis, const char *title);

    void setAxisUnits(Axis axis, const char *units);

    // Colormaps

    void addColorMap(const ColorMapId& id, ColorMap *colorMap);

    void deleteColorMap(const ColorMapId& id);

    ColorMap *getColorMap(const ColorMapId& id);

    bool renderColorMap(const ColorMapId& id, 
                        const DataSetId& dataSetID,
                        LegendType legendType,
                        std::string& title,
                        double range[2],
                        int width, int height,
                        int numLabels,
                        vtkUnsignedCharArray *imgData);

    // 2D Contour plots

    bool addContour2D(const DataSetId& id, int numContours);

    bool addContour2D(const DataSetId& id, const std::vector<double>& contours);

    void deleteContour2D(const DataSetId& id);

    Contour2D *getContour2D(const DataSetId& id);

    void setContour2DTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setContour2DOrientation(const DataSetId& id, double quat[4]);

    void setContour2DOrientation(const DataSetId& id, double angle, double axis[3]);

    void setContour2DPosition(const DataSetId& id, double pos[3]);

    void setContour2DScale(const DataSetId& id, double scale[3]);

    void setContour2DColor(const DataSetId& id, float color[3]);

    void setContour2DEdgeWidth(const DataSetId& id, float edgeWidth);

    void setContour2DLighting(const DataSetId& id, bool state);
    
    void setContour2DOpacity(const DataSetId& id, double opacity);

    void setContour2DVisibility(const DataSetId& id, bool state);

    void setContour2DContours(const DataSetId& id, int numContours);

    void setContour2DContourList(const DataSetId& id, const std::vector<double>& contours);

    // 3D Contour (isosurface) plots

    bool addContour3D(const DataSetId& id, int numContours);

    bool addContour3D(const DataSetId& id, const std::vector<double>& contours);

    void deleteContour3D(const DataSetId& id);

    Contour3D *getContour3D(const DataSetId& id);

    void setContour3DTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setContour3DOrientation(const DataSetId& id, double quat[4]);

    void setContour3DOrientation(const DataSetId& id, double angle, double axis[3]);

    void setContour3DPosition(const DataSetId& id, double pos[3]);

    void setContour3DScale(const DataSetId& id, double scale[3]);

    void setContour3DColor(const DataSetId& id, float color[3]);

    void setContour3DEdgeVisibility(const DataSetId& id, bool state);

    void setContour3DEdgeColor(const DataSetId& id, float color[3]);

    void setContour3DEdgeWidth(const DataSetId& id, float edgeWidth);

    void setContour3DLighting(const DataSetId& id, bool state);

    void setContour3DOpacity(const DataSetId& id, double opacity);

    void setContour3DVisibility(const DataSetId& id, bool state);

    void setContour3DWireframe(const DataSetId& id, bool state);

    void setContour3DContours(const DataSetId& id, int numContours);

    void setContour3DContourList(const DataSetId& id, const std::vector<double>& contours);

    void setContour3DColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    // Cutplanes

    bool addCutplane(const DataSetId& id);
    
    void deleteCutplane(const DataSetId& id);

    Cutplane *getCutplane(const DataSetId& id);

    void setCutplaneTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setCutplaneOrientation(const DataSetId& id, double quat[4]);

    void setCutplaneOrientation(const DataSetId& id, double angle, double axis[3]);

    void setCutplanePosition(const DataSetId& id, double pos[3]);

    void setCutplaneScale(const DataSetId& id, double scale[3]);

    void setCutplaneEdgeVisibility(const DataSetId& id, bool state);

    void setCutplaneEdgeColor(const DataSetId& id, float color[3]);

    void setCutplaneEdgeWidth(const DataSetId& id, float edgeWidth);

    void setCutplaneLighting(const DataSetId& id, bool state);

    void setCutplaneOpacity(const DataSetId& id, double opacity);

    void setCutplaneVisibility(const DataSetId& id, bool state);

    void setCutplaneWireframe(const DataSetId& id, bool state);

    void setCutplaneVolumeSlice(const DataSetId& id, Cutplane::Axis axis, double ratio);

    void setCutplaneColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setCutplaneColorMode(const DataSetId& id, Cutplane::ColorMode mode);

    // Glyphs

    bool addGlyphs(const DataSetId& id, Glyphs::GlyphShape shape);

    void deleteGlyphs(const DataSetId& id);

    Glyphs *getGlyphs(const DataSetId& id);

    void setGlyphsTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setGlyphsOrientation(const DataSetId& id, double quat[4]);

    void setGlyphsOrientation(const DataSetId& id, double angle, double axis[3]);

    void setGlyphsPosition(const DataSetId& id, double pos[3]);

    void setGlyphsScale(const DataSetId& id, double scale[3]);

    void setGlyphsColor(const DataSetId& id, float color[3]);

    void setGlyphsEdgeVisibility(const DataSetId& id, bool state);

    void setGlyphsEdgeColor(const DataSetId& id, float color[3]);

    void setGlyphsEdgeWidth(const DataSetId& id, float edgeWidth);

    void setGlyphsLighting(const DataSetId& id, bool state);

    void setGlyphsOpacity(const DataSetId& id, double opacity);

    void setGlyphsVisibility(const DataSetId& id, bool state);

    void setGlyphsWireframe(const DataSetId& id, bool state);

    void setGlyphsColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setGlyphsShape(const DataSetId& id, Glyphs::GlyphShape shape);

    void setGlyphsColorMode(const DataSetId& id, Glyphs::ColorMode mode);

    void setGlyphsScalingMode(const DataSetId& id, Glyphs::ScalingMode mode);

    void setGlyphsNormalizeScale(const DataSetId& id, bool normalize);

    void setGlyphsScaleFactor(const DataSetId& id, double scale);

    // Height maps

    bool addHeightMap(const DataSetId& id, int numContours, double heightScale);

    bool addHeightMap(const DataSetId& id, const std::vector<double>& contours, double heightScale);

    void deleteHeightMap(const DataSetId& id);

    HeightMap *getHeightMap(const DataSetId& id);

    void setHeightMapTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setHeightMapOrientation(const DataSetId& id, double quat[4]);

    void setHeightMapOrientation(const DataSetId& id, double angle, double axis[3]);

    void setHeightMapPosition(const DataSetId& id, double pos[3]);

    void setHeightMapScale(const DataSetId& id, double scale[3]);
    
    void setHeightMapEdgeVisibility(const DataSetId& id, bool state);

    void setHeightMapEdgeColor(const DataSetId& id, float color[3]);

    void setHeightMapEdgeWidth(const DataSetId& id, float edgeWidth);

    void setHeightMapLighting(const DataSetId& id, bool state);

    void setHeightMapOpacity(const DataSetId& id, double opacity);

    void setHeightMapVisibility(const DataSetId& id, bool state);

    void setHeightMapWireframe(const DataSetId& id, bool state);

    void setHeightMapVolumeSlice(const DataSetId& id, HeightMap::Axis axis, double ratio);

    void setHeightMapHeightScale(const DataSetId& id, double scale);

    void setHeightMapColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setHeightMapNumContours(const DataSetId& id, int numContours);

    void setHeightMapContourList(const DataSetId& id, const std::vector<double>& contours);

    void setHeightMapContourSurfaceVisibility(const DataSetId& id, bool state);

    void setHeightMapContourLineVisibility(const DataSetId& id, bool state);

    void setHeightMapContourEdgeColor(const DataSetId& id, float color[3]);

    void setHeightMapContourEdgeWidth(const DataSetId& id, float edgeWidth);

    // LIC plots

    bool addLIC(const DataSetId& id);
    
    void deleteLIC(const DataSetId& id);

    LIC *getLIC(const DataSetId& id);

    void setLICTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setLICOrientation(const DataSetId& id, double quat[4]);

    void setLICOrientation(const DataSetId& id, double angle, double axis[3]);

    void setLICPosition(const DataSetId& id, double pos[3]);

    void setLICScale(const DataSetId& id, double scale[3]);

    void setLICEdgeVisibility(const DataSetId& id, bool state);
    
    void setLICEdgeColor(const DataSetId& id, float color[3]);

    void setLICEdgeWidth(const DataSetId& id, float edgeWidth);

    void setLICLighting(const DataSetId& id, bool state);

    void setLICOpacity(const DataSetId& id, double opacity);

    void setLICVisibility(const DataSetId& id, bool state);

    void setLICVolumeSlice(const DataSetId& id, LIC::Axis axis, double ratio);

    void setLICColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    // Molecules

    bool addMolecule(const DataSetId& id);
    
    void deleteMolecule(const DataSetId& id);

    Molecule *getMolecule(const DataSetId& id);

    void setMoleculeTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setMoleculeOrientation(const DataSetId& id, double quat[4]);

    void setMoleculeOrientation(const DataSetId& id, double angle, double axis[3]);

    void setMoleculePosition(const DataSetId& id, double pos[3]);

    void setMoleculeScale(const DataSetId& id, double scale[3]);
    
    void setMoleculeEdgeVisibility(const DataSetId& id, bool state);

    void setMoleculeEdgeColor(const DataSetId& id, float color[3]);

    void setMoleculeEdgeWidth(const DataSetId& id, float edgeWidth);

    void setMoleculeLighting(const DataSetId& id, bool state);

    void setMoleculeOpacity(const DataSetId& id, double opacity);

    void setMoleculeVisibility(const DataSetId& id, bool state);

    void setMoleculeWireframe(const DataSetId& id, bool state);

    void setMoleculeColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setMoleculeAtomScaling(const DataSetId& id, Molecule::AtomScaling scaling);

    void setMoleculeAtomVisibility(const DataSetId& id, bool state);

    void setMoleculeBondVisibility(const DataSetId& id, bool state);

    // PolyData Meshes

    bool addPolyData(const DataSetId& id);
    
    void deletePolyData(const DataSetId& id);

    PolyData *getPolyData(const DataSetId& id);

    void setPolyDataTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setPolyDataOrientation(const DataSetId& id, double quat[4]);

    void setPolyDataOrientation(const DataSetId& id, double angle, double axis[3]);

    void setPolyDataPosition(const DataSetId& id, double pos[3]);

    void setPolyDataScale(const DataSetId& id, double scale[3]);

    void setPolyDataColor(const DataSetId& id, float color[3]);

    void setPolyDataEdgeVisibility(const DataSetId& id, bool state);

    void setPolyDataEdgeColor(const DataSetId& id, float color[3]);

    void setPolyDataEdgeWidth(const DataSetId& id, float edgeWidth);

    void setPolyDataPointSize(const DataSetId& id, float size);

    void setPolyDataLighting(const DataSetId& id, bool state);

    void setPolyDataOpacity(const DataSetId& id, double opacity);

    void setPolyDataVisibility(const DataSetId& id, bool state);

    void setPolyDataWireframe(const DataSetId& id, bool state);

    // Color-mapped surfaces

    bool addPseudoColor(const DataSetId& id);

    void deletePseudoColor(const DataSetId& id);

    PseudoColor *getPseudoColor(const DataSetId& id);

    void setPseudoColorTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setPseudoColorOrientation(const DataSetId& id, double quat[4]);

    void setPseudoColorOrientation(const DataSetId& id, double angle, double axis[3]);

    void setPseudoColorPosition(const DataSetId& id, double pos[3]);

    void setPseudoColorScale(const DataSetId& id, double scale[3]);

    void setPseudoColorColor(const DataSetId& id, float color[3]);

    void setPseudoColorEdgeVisibility(const DataSetId& id, bool state);

    void setPseudoColorEdgeColor(const DataSetId& id, float color[3]);

    void setPseudoColorEdgeWidth(const DataSetId& id, float edgeWidth);

    void setPseudoColorLighting(const DataSetId& id, bool state);
    
    void setPseudoColorOpacity(const DataSetId& id, double opacity);

    void setPseudoColorVisibility(const DataSetId& id, bool state);

    void setPseudoColorWireframe(const DataSetId& id, bool state);

    void setPseudoColorColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setPseudoColorColorMode(const DataSetId& id, PseudoColor::ColorMode mode);

    // Streamlines

    bool addStreamlines(const DataSetId& id);

    void deleteStreamlines(const DataSetId& id);

    Streamlines *getStreamlines(const DataSetId& id);

    void setStreamlinesTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setStreamlinesOrientation(const DataSetId& id, double quat[4]);

    void setStreamlinesOrientation(const DataSetId& id, double angle, double axis[3]);

    void setStreamlinesPosition(const DataSetId& id, double pos[3]);

    void setStreamlinesScale(const DataSetId& id, double scale[3]);

    void setStreamlinesColor(const DataSetId& id, float color[3]);

    void setStreamlinesEdgeVisibility(const DataSetId& id, bool state);

    void setStreamlinesEdgeColor(const DataSetId& id, float color[3]);

    void setStreamlinesEdgeWidth(const DataSetId& id, float edgeWidth);

    void setStreamlinesLighting(const DataSetId& id, bool state);

    void setStreamlinesOpacity(const DataSetId& id, double opacity);

    void setStreamlinesVisibility(const DataSetId& id, bool state);

    void setStreamlinesSeedToMeshPoints(const DataSetId& id);

    void setStreamlinesSeedToFilledMesh(const DataSetId& id, int numPoints);

    bool setStreamlinesSeedToMeshPoints(const DataSetId& id,
                                        char *data, size_t nbytes);

    bool setStreamlinesSeedToFilledMesh(const DataSetId& id,
                                        char *data, size_t nbytes,
                                        int numPoints);

    void setStreamlinesSeedToRake(const DataSetId& id,
                                  double start[3], double end[3],
                                  int numPoints);

    void setStreamlinesSeedToDisk(const DataSetId& id,
                                  double center[3], double normal[3],
                                  double radius, double innerRadius,
                                  int numPoints);

    void setStreamlinesSeedToPolygon(const DataSetId& id,
                                     double center[3], double normal[3],
                                     double angle, double radius,
                                     int numSides);

    void setStreamlinesSeedToFilledPolygon(const DataSetId& id,
                                           double center[3], double normal[3],
                                           double angle, double radius,
                                           int numSides, int numPoints);

    void setStreamlinesLength(const DataSetId& id, double length);

    void setStreamlinesTypeToLines(const DataSetId& id);

    void setStreamlinesTypeToTubes(const DataSetId& id, int numSides, double radius);

    void setStreamlinesTypeToRibbons(const DataSetId& id, double width, double angle);

    void setStreamlinesSeedVisibility(const DataSetId& id, bool state);

    void setStreamlinesColorMode(const DataSetId& id, Streamlines::ColorMode mode);

    void setStreamlinesColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    void setStreamlinesSeedColor(const DataSetId& id, float color[3]);

    // Volumes

    bool addVolume(const DataSetId& id);

    void deleteVolume(const DataSetId& id);

    Volume *getVolume(const DataSetId& id);

    void setVolumeTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    void setVolumeOrientation(const DataSetId& id, double quat[4]);

    void setVolumeOrientation(const DataSetId& id, double angle, double axis[3]);

    void setVolumePosition(const DataSetId& id, double pos[3]);

    void setVolumeScale(const DataSetId& id, double scale[3]);

    void setVolumeAmbient(const DataSetId& id, double coeff);

    void setVolumeDiffuse(const DataSetId& id, double coeff);

    void setVolumeSpecular(const DataSetId& id, double coeff, double power);

    void setVolumeLighting(const DataSetId& id, bool state);
    
    void setVolumeOpacity(const DataSetId& id, double opacity);

    void setVolumeVisibility(const DataSetId& id, bool state);

    void setVolumeColorMap(const DataSetId& id, const ColorMapId& colorMapId);

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

    void collectDataRanges();

    void collectScalarRanges(double *range, bool onlyVisible);

    void collectVectorMagnitudeRanges(double *range, bool onlyVisible);

    void collectVectorComponentRanges(double *range, int component, bool onlyVisible);

    void updateRanges();

    void updateColorMap(ColorMap *cmap);

    bool colorMapUsed(ColorMap *cmap);

    void setCameraFromMatrix(vtkCamera *camera, vtkMatrix4x4 &mat);

    void computeDisplayToWorld(double x, double y, double z, double worldPt[4]);

    void computeWorldToDisplay(double x, double y, double z, double displayPt[3]);

    void computeScreenWorldCoords();

    bool is2D(const double bounds[6],
              PrincipalPlane *plane,
              double *offset) const;
    void initCamera();
    void initAxes();
    void resetAxes(double bounds[6] = NULL);
    void setCameraClippingPlanes();

    bool _needsRedraw;
    int _windowWidth, _windowHeight;
    double _imgWorldOrigin[2];
    double _imgWorldDims[2];
    PrincipalPlane _imgCameraPlane;
    double _imgCameraOffset;
    double _screenWorldCoords[4];
    double _cameraOrientation[4];
    double _cameraZoomRatio;
    double _cameraPan[2];
    float _bgColor[3];
    bool _useCumulativeRange;
    bool _cumulativeRangeOnlyVisible;
    double _cumulativeScalarRange[2];
    double _cumulativeVectorMagnitudeRange[2];
    double _cumulativeVectorComponentRange[3][2];

    ColorMapHashmap _colorMaps;
    DataSetHashmap _dataSets;
    Contour2DHashmap _contour2Ds;
    Contour3DHashmap _contour3Ds;
    CutplaneHashmap _cutplanes;
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
