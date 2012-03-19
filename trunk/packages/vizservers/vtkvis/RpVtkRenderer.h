/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_RENDERER_H__
#define __RAPPTURE_VTKVIS_RENDERER_H__

#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <typeinfo>

#include <vtkSmartPointer.h>
#ifdef USE_CUSTOM_AXES
#include "vtkRpCubeAxesActor.h"
#include "vtkRpCubeAxesActor2D.h"
#else
#include <vtkCubeAxesActor.h>
#include <vtkCubeAxesActor2D.h>
#endif
#include <vtkScalarBarActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>

#include "ColorMap.h"
#include "RpTypes.h"
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
        LEGEND_SCALAR,
        LEGEND_VECTOR_MAGNITUDE,
        LEGEND_VECTOR_X,
        LEGEND_VECTOR_Y,
        LEGEND_VECTOR_Z
    };

    typedef std::string DataSetId;
    typedef std::string ColorMapId;
    typedef std::string FieldId;

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

    bool getUseCumulativeRange();

    bool getCumulativeDataRange(double *range, const char *name,
                                int numComponents,
                                int component = -1);

    bool getCumulativeDataRange(double *range, const char *name,
                                DataSet::DataAttributeType type,
                                int numComponents,
                                int component = -1);

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
                        const char *fieldName,
                        DataSet::DataAttributeType type,
                        std::string& title,
                        double range[2],
                        int width, int height,
                        bool opaque,
                        int numLabels,
                        vtkUnsignedCharArray *imgData);

    bool renderColorMap(const ColorMapId& id, 
                        const DataSetId& dataSetID,
                        LegendType legendType,
                        const char *fieldName,
                        std::string& title,
                        double range[2],
                        int width, int height,
                        bool opaque,
                        int numLabels,
                        vtkUnsignedCharArray *imgData);

    bool renderColorMap(const ColorMapId& id, 
                        const DataSetId& dataSetID,
                        LegendType legendType,
                        std::string& title,
                        double range[2],
                        int width, int height,
                        bool opaque,
                        int numLabels,
                        vtkUnsignedCharArray *imgData);

    // Generic VtkGraphicsObject methods

    template<class GraphicsObject>
    GraphicsObject *getGraphicsObject(const DataSetId& id);

    template<class GraphicsObject>
    bool addGraphicsObject(const DataSetId& id);

    template<class GraphicsObject>
    void deleteGraphicsObject(const DataSetId& id);

    template<class GraphicsObject>
    void setGraphicsObjectColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    template<class GraphicsObject>
    void setGraphicsObjectVolumeSlice(const DataSetId& id, Axis axis, double ratio);

    //   Prop/Prop3D properties

    template<class GraphicsObject>
    void setGraphicsObjectOrientation(const DataSetId& id, double quat[4]);

    template<class GraphicsObject>
    void setGraphicsObjectOrientation(const DataSetId& id, double angle, double axis[3]);

    template<class GraphicsObject>
    void setGraphicsObjectPosition(const DataSetId& id, double pos[3]);

    template<class GraphicsObject>
    void setGraphicsObjectScale(const DataSetId& id, double scale[3]);

    template<class GraphicsObject>
    void setGraphicsObjectTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    template<class GraphicsObject>
    void setGraphicsObjectVisibility(const DataSetId& id, bool state);

    //   Actor properties

    template<class GraphicsObject>
    void setGraphicsObjectColor(const DataSetId& id, float color[3]);

    template<class GraphicsObject>
    void setGraphicsObjectEdgeVisibility(const DataSetId& id, bool state);

    template<class GraphicsObject>
    void setGraphicsObjectEdgeColor(const DataSetId& id, float color[3]);

    template<class GraphicsObject>
    void setGraphicsObjectEdgeWidth(const DataSetId& id, float edgeWidth);

    template<class GraphicsObject>
    void setGraphicsObjectAmbient(const DataSetId& id, double coeff);

    template<class GraphicsObject>
    void setGraphicsObjectDiffuse(const DataSetId& id, double coeff);

    template<class GraphicsObject>
    void setGraphicsObjectSpecular(const DataSetId& id, double coeff, double power);

    template<class GraphicsObject>
    void setGraphicsObjectLighting(const DataSetId& id, bool state);

    template<class GraphicsObject>
    void setGraphicsObjectOpacity(const DataSetId& id, double opacity);

    template<class GraphicsObject>
    void setGraphicsObjectPointSize(const DataSetId& id, float size);

    template<class GraphicsObject>
    void setGraphicsObjectWireframe(const DataSetId& id, bool state);

    // 2D Contour plots

    bool addContour2D(const DataSetId& id, int numContours);

    bool addContour2D(const DataSetId& id, const std::vector<double>& contours);

    void setContour2DContours(const DataSetId& id, int numContours);

    void setContour2DContourList(const DataSetId& id, const std::vector<double>& contours);

    // 3D Contour (isosurface) plots

    bool addContour3D(const DataSetId& id, int numContours);

    bool addContour3D(const DataSetId& id, const std::vector<double>& contours);

    void setContour3DContours(const DataSetId& id, int numContours);

    void setContour3DContourList(const DataSetId& id, const std::vector<double>& contours);

    // Cutplanes

    void setCutplaneOutlineVisibility(const DataSetId& id, bool state);

    void setCutplaneSliceVisibility(const DataSetId& id, Axis axis, bool state);

    void setCutplaneColorMode(const DataSetId& id,
                              Cutplane::ColorMode mode,
                              const char *name, double range[2] = NULL);

    void setCutplaneColorMode(const DataSetId& id,
                              Cutplane::ColorMode mode,
                              DataSet::DataAttributeType type,
                              const char *name, double range[2] = NULL);

    // Glyphs

    bool addGlyphs(const DataSetId& id, Glyphs::GlyphShape shape);

    void setGlyphsShape(const DataSetId& id, Glyphs::GlyphShape shape);

    void setGlyphsColorMode(const DataSetId& id,
                            Glyphs::ColorMode mode,
                            const char *name,
                            double range[2] = NULL);

    void setGlyphsScalingMode(const DataSetId& id,
                              Glyphs::ScalingMode mode,
                              const char *name,
                              double range[2] = NULL);

    void setGlyphsNormalizeScale(const DataSetId& id, bool normalize);

    void setGlyphsScaleFactor(const DataSetId& id, double scale);

    // Height maps

    bool addHeightMap(const DataSetId& id, int numContours, double heightScale);

    bool addHeightMap(const DataSetId& id, const std::vector<double>& contours, double heightScale);

    void setHeightMapHeightScale(const DataSetId& id, double scale);

    void setHeightMapNumContours(const DataSetId& id, int numContours);

    void setHeightMapContourList(const DataSetId& id, const std::vector<double>& contours);

    void setHeightMapContourSurfaceVisibility(const DataSetId& id, bool state);

    void setHeightMapContourLineVisibility(const DataSetId& id, bool state);

    void setHeightMapContourEdgeColor(const DataSetId& id, float color[3]);

    void setHeightMapContourEdgeWidth(const DataSetId& id, float edgeWidth);

    // Molecules

    void setMoleculeAtomScaling(const DataSetId& id, Molecule::AtomScaling scaling);

    void setMoleculeAtomVisibility(const DataSetId& id, bool state);

    void setMoleculeBondVisibility(const DataSetId& id, bool state);

    // Color-mapped surfaces

    void setPseudoColorColorMode(const DataSetId& id,
                                 PseudoColor::ColorMode mode,
                                 const char *name, double range[2] = NULL);

    void setPseudoColorColorMode(const DataSetId& id,
                                 PseudoColor::ColorMode mode,
                                 DataSet::DataAttributeType type,
                                 const char *name, double range[2] = NULL);

    // Streamlines

    void setStreamlinesNumberOfSeedPoints(const DataSetId& id, int numPoints);

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

    void setStreamlinesColorMode(const DataSetId& id,
                                 Streamlines::ColorMode mode,
                                 const char *name, double range[2] = NULL);

    void setStreamlinesColorMode(const DataSetId& id,
                                 Streamlines::ColorMode mode,
                                 DataSet::DataAttributeType type,
                                 const char *name, double range[2] = NULL);

    void setStreamlinesSeedColor(const DataSetId& id, float color[3]);

    // Volumes

    void setVolumeSampleDistance(const DataSetId& id, double distance);

private:
    typedef std::tr1::unordered_map<DataSetId, DataSet *> DataSetHashmap;
    typedef std::tr1::unordered_map<FieldId, double *> FieldRangeHashmap;
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

    static void printCameraInfo(vtkCamera *camera);

    static void setCameraFromMatrix(vtkCamera *camera, vtkMatrix4x4 &mat);

    static void mergeBounds(double *boundsDest, const double *bounds1, const double *bounds2);

    template<class GraphicsObject>
    std::tr1::unordered_map<DataSetId, GraphicsObject *>&getGraphicsObjectHashmap();

    void collectBounds(double *bounds, bool onlyVisible);

    void collectDataRanges();

    void collectDataRanges(double *range, const char *name,
                           DataSet::DataAttributeType type,
                           int component, bool onlyVisible);

    void clearFieldRanges();

    void initFieldRanges();

    void updateFieldRanges();

    void updateColorMap(ColorMap *cmap);

    bool colorMapUsed(ColorMap *cmap);

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

    FieldRangeHashmap _scalarPointDataRange;
    FieldRangeHashmap _vectorPointDataRange;
    FieldRangeHashmap _vectorCompPointDataRange[3];
    FieldRangeHashmap _scalarCellDataRange;
    FieldRangeHashmap _vectorCellDataRange;
    FieldRangeHashmap _vectorCompCellDataRange[3];

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
#ifdef USE_CUSTOM_AXES
    vtkSmartPointer<vtkRpCubeAxesActor> _cubeAxesActor; // For 3D view
    vtkSmartPointer<vtkRpCubeAxesActor2D> _cubeAxesActor2D; // For 2D view
#else
    vtkSmartPointer<vtkCubeAxesActor> _cubeAxesActor; // For 3D view
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
