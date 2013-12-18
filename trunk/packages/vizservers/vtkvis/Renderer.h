/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_RENDERER_H
#define VTKVIS_RENDERER_H

#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <typeinfo>

#include <vtkSmartPointer.h>
#ifdef USE_CUSTOM_AXES
#include "vtkRpCubeAxesActor.h"
#else
#include <vtkCubeAxesActor.h>
#endif
#include <vtkAxesActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkScalarBarActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>

#include "ColorMap.h"
#include "Types.h"
#include "DataSet.h"
#include "Arc.h"
#include "Arrow.h"
#include "Box.h"
#include "Cone.h"
#include "Contour2D.h"
#include "Contour3D.h"
#include "Cutplane.h"
#include "Cylinder.h"
#include "Disk.h"
#include "Glyphs.h"
#include "Group.h"
#include "HeightMap.h"
#include "Image.h"
#include "ImageCutplane.h"
#include "LIC.h"
#include "Line.h"
#include "Molecule.h"
#include "Outline.h"
#include "Parallelepiped.h"
#include "PolyData.h"
#include "Polygon.h"
#include "PseudoColor.h"
#include "Sphere.h"
#include "Streamlines.h"
#include "Text3D.h"
#include "Volume.h"
#include "Warp.h"
#include "Trace.h"

// Controls if TGA format is sent to client
//#define RENDER_TARGA
#define TARGA_BYTES_PER_PIXEL 3

namespace VtkVis {

/**
 * \brief VTK Renderer
 */
class Renderer
{
public:
    Renderer();
    virtual ~Renderer();

    enum AxisRangeMode {
        RANGE_AUTO = 0,
        RANGE_SCALE_BOUNDS,
        RANGE_EXPLICIT
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

    enum Aspect {
        ASPECT_NATIVE,
        ASPECT_SQUARE,
        ASPECT_WINDOW
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

    void setDataSetOpacity(const DataSetId& id, double opacity);

    void setDataSetVisibility(const DataSetId& id, bool state);

    void setUseCumulativeDataRange(bool state, bool onlyVisible = false);

    bool getUseCumulativeRange();

    bool setCumulativeDataRange(double *range, const char *name,
                                DataSet::DataAttributeType type,
                                int numComponents,
                                int component = -1);

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

    // Lights

    int addLight(float pos[3]);

    vtkLight *getLight(int lightIdx);

    void setLightSwitch(int lightIdx, bool state);

    // Camera controls

    void setViewAngle(int height);

    /// Return the VTK camera object this Renderer uses
    vtkCamera *getVtkCamera()
    {
        if (_renderer != NULL)
            return _renderer->GetActiveCamera();
        else
            return NULL;
    }

    bool isCameraMaximized()
    {
        return (_cameraZoomRatio == 1.0 &&
                _cameraPan[0] == 0.0 &&
                _cameraPan[1] == 0.0);
    }

    void getImageCameraSizes(int *imgWidthPx, int *imgHeightPx,
                             int *_pxOffsetX = NULL, int *_pxOffsetY = NULL);

    double getImageCameraAspect();

    void setCameraMode(CameraMode mode);

    CameraMode getCameraMode() const;

    void setCameraAspect(Aspect aspect);

    void resetVtkCamera(double *bounds = NULL);

    void resetCamera(bool resetOrientation = true);

    void resetCameraClippingRange();

    bool setCameraZoomRegionPixels(int x, int y, int width, int height);

    bool setCameraZoomRegion(double x, double y, double width, double height);

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

    void eventuallyResetCamera()
    {
        _needsCameraReset = true;
    }

    bool needsCameraReset()
    {
        return _needsCameraReset;
    }

    void eventuallyResetCameraClippingRange()
    {
        _needsCameraClippingRangeReset = true;
    }

    bool needsCameraClippingRangeReset()
    {
        return _needsCameraClippingRangeReset;
    }

    // Rendering an image

    void setBackgroundColor(float color[3]);

    void setClipPlane(Axis axis, double ratio, int direction);

    void setUseTwoSidedLighting(bool state);

    void setUseDepthPeeling(bool state);

    void setDepthPeelingParams(double occlusionRatio = 0.0, int maxPeels = 0);

    void eventuallyRender();

    bool render();

    void getRenderedFrame(vtkUnsignedCharArray *imgData);

    // Axes

    void setAxesOrigin(double x, double y, double z, bool useCustom = true);

    void setAxesAutoBounds(bool state);

    void setAxesBounds(double min, double max);

    void setAxesAutoRange(bool state);

    void setAxisBounds(Axis axis, double min, double max);

    void setAxesScale(double scale);

    void setAxesRange(double min, double max);

    void setAxesLabelPowerScaling(int xPow, int yPow, int zPow, bool useCustom = true);

    void setAxisAutoBounds(Axis axis, bool state);

    void setAxisAutoRange(Axis axis, bool state);

    void setAxisScale(Axis axis, double scale);

    void setAxisRange(Axis axis, double min, double max);

    void setAxesFlyMode(AxesFlyMode mode);

    void setAxesVisibility(bool state);

    void setAxesGridVisibility(bool state);

    void setAxesInnerGridVisibility(bool state);

    void setAxesGridpolysVisibility(bool state);

    void setAxesLabelVisibility(bool state);

    void setAxesTickVisibility(bool state);

    void setAxesMinorTickVisibility(bool state);

    void setAxesTickPosition(AxesTickPosition pos);

    void setAxesColor(double color[3], double opacity = 1.0);

    void setAxesTitleColor(double color[3], double opacity = 1.0);

    void setAxesLabelColor(double color[3], double opacity = 1.0);

    void setAxesLinesColor(double color[3], double opacity = 1.0);

    void setAxesGridlinesColor(double color[3], double opacity = 1.0);

    void setAxesInnerGridlinesColor(double color[3], double opacity = 1.0);

    void setAxesGridpolysColor(double color[3], double opacity = 1.0);

    void setAxesLabelScaling(bool autoScale, int xpow, int ypow, int zpow);

    void setAxesPixelFontSize(double screenSize);

    void setAxesTitleFont(const char *fontName);

    void setAxesTitleFontSize(int sz);

    void setAxesTitleOrientation(double orientation);

    void setAxesLabelFont(const char *fontName);

    void setAxesLabelFontSize(int sz);

    void setAxesLabelOrientation(double orientation);

    void setAxesLabelFormat(const char *format);

    void setAxisVisibility(Axis axis, bool state);

    void setAxisGridVisibility(Axis axis, bool state);

    void setAxisInnerGridVisibility(Axis axis, bool state);

    void setAxisGridpolysVisibility(Axis axis, bool state);

    void setAxisLabelVisibility(Axis axis, bool state);

    void setAxisTickVisibility(Axis axis, bool state);

    void setAxisMinorTickVisibility(Axis axis, bool state);

    void setAxisColor(Axis axis, double color[3], double opacity = 1.0);

    void setAxisTitleColor(Axis axis, double color[3], double opacity = 1.0);

    void setAxisLabelColor(Axis axis, double color[3], double opacity = 1.0);

    void setAxisLinesColor(Axis axis, double color[3], double opacity = 1.0);

    void setAxisGridlinesColor(Axis axis, double color[3], double opacity = 1.0);

    void setAxisInnerGridlinesColor(Axis axis, double color[3], double opacity = 1.0);

    void setAxisGridpolysColor(Axis axis, double color[3], double opacity = 1.0);

    void setAxisTitle(Axis axis, const char *title);

    void setAxisUnits(Axis axis, const char *units);

    void setAxisTitleFont(Axis axis, const char *fontName);

    void setAxisTitleFontSize(Axis axis, int sz);

    void setAxisTitleOrientation(Axis axis, double orientation);

    void setAxisLabelFont(Axis axis, const char *fontName);

    void setAxisLabelFontSize(Axis axis, int sz);

    void setAxisLabelOrientation(Axis axis, double orientation);

    void setAxisLabelFormat(Axis axis, const char *format);

    void eventuallyResetAxes()
    {
        _needsAxesReset = true;
    }

    bool needsAxesReset()
    {
        return _needsAxesReset;
    }

    // Colormaps

    void addColorMap(const ColorMapId& id, ColorMap *colorMap);

    void deleteColorMap(const ColorMapId& id);

    ColorMap *getColorMap(const ColorMapId& id);

    void setColorMapNumberOfTableEntries(const ColorMapId& id, int numEntries);

    bool renderColorMap(const ColorMapId& id,
                        int width, int height,
                        bool opaque,
                        vtkUnsignedCharArray *imgData);

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

    // Generic GraphicsObject methods

    GraphicsObject *getGenericGraphicsObject(const DataSetId& id);

    template<class T>
    T *getGraphicsObject(const DataSetId& id);

    template<class T>
    bool addGraphicsObject(const DataSetId& id);

    template<class T>
    void deleteGraphicsObject(const DataSetId& id);

    template<class T>
    void deleteAllGraphicsObjects();

    template<class T>
    void mergeGraphicsObjectBounds(double *bounds, bool onlyVisible);

    template<class T>
    void mergeGraphicsObjectUnscaledBounds(double *bounds, bool onlyVisible);

    template<class T>
    void updateGraphicsObjectFieldRanges();

    template<class T>
    void setGraphicsObjectClippingPlanes(vtkPlaneCollection *planes);

    template<class T>
    void setGraphicsObjectAspect(double aspectRatio);

    template<class T>
    void setGraphicsObjectInterpolateBeforeMapping(const DataSetId& id, bool state);

    template<class T>
    void setGraphicsObjectColorMap(const DataSetId& id, const ColorMapId& colorMapId);

    template<class T>
    void updateGraphicsObjectColorMap(ColorMap *cmap);

    template<class T>
    bool graphicsObjectColorMapUsed(ColorMap *cmap);

    template<class T>
    void setGraphicsObjectVolumeSlice(const DataSetId& id, Axis axis, double ratio);

    // Prop/Prop3D properties

    template<class T>
    void setGraphicsObjectOrientation(const DataSetId& id, double quat[4]);

    template<class T>
    void setGraphicsObjectOrientation(const DataSetId& id, double angle, double axis[3]);

    template<class T>
    void setGraphicsObjectOrigin(const DataSetId& id, double origin[3]);

    template<class T>
    void setGraphicsObjectPosition(const DataSetId& id, double pos[3]);

    template<class T>
    void setGraphicsObjectAspect(const DataSetId& id, double aspect);

    template<class T>
    void setGraphicsObjectScale(const DataSetId& id, double scale[3]);

    template<class T>
    void setGraphicsObjectTransform(const DataSetId& id, vtkMatrix4x4 *trans);

    template<class T>
    void setGraphicsObjectVisibility(const DataSetId& id, bool state);

    // Actor properties

    template<class T>
    void setGraphicsObjectColor(const DataSetId& id, float color[3]);

    template<class T>
    void setGraphicsObjectCullFace(const DataSetId& id, GraphicsObject::CullFace state);

    template<class T>
    void setGraphicsObjectCulling(const DataSetId& id, bool state);

    template<class T>
    void setGraphicsObjectEdgeVisibility(const DataSetId& id, bool state);

    template<class T>
    void setGraphicsObjectEdgeColor(const DataSetId& id, float color[3]);

    template<class T>
    void setGraphicsObjectEdgeWidth(const DataSetId& id, float edgeWidth);

    template<class T>
    void setGraphicsObjectAmbient(const DataSetId& id, double coeff);

    template<class T>
    void setGraphicsObjectDiffuse(const DataSetId& id, double coeff);

    template<class T>
    void setGraphicsObjectShadingModel(const DataSetId& id, GraphicsObject::ShadingModel state);

    template<class T>
    void setGraphicsObjectSpecular(const DataSetId& id, double coeff, double power);

    template<class T>
    void setGraphicsObjectLighting(const DataSetId& id, bool state);

    template<class T>
    void setGraphicsObjectOpacity(const DataSetId& id, double opacity);

    template<class T>
    void setGraphicsObjectPointSize(const DataSetId& id, float size);

    template<class T>
    void setGraphicsObjectWireframe(const DataSetId& id, bool state);

    // For Shapes:

    template<class T>
    void setGraphicsObjectFlipNormals(const DataSetId& id, bool state);

    // Arcs

    bool addArc(const DataSetId& id, double center[3], double pt1[3],
                double normal[3], double angle);

    void setArcResolution(const DataSetId& id, int res);

    // Arrows

    bool addArrow(const DataSetId& id, double tipRadius, double shaftRadius,
                  double tipLength, bool flipNormals = false);

    void setArrowResolution(const DataSetId& id, int resTip, int resShaft);

    // Boxes

    bool addBox(const DataSetId& id, double xLen, double yLen, double zLen,
                bool flipNormals = false);

    // Cones

    bool addCone(const DataSetId& id, double radius, double height, bool cap,
                 bool flipNormals = false);

    void setConeResolution(const DataSetId& id, int res);

    // 2D Contour plots

    bool addContour2D(const DataSetId& id, int numContours);

    bool addContour2D(const DataSetId& id, const std::vector<double>& contours);

    void setContour2DContourField(const DataSetId& id, const char *fieldName);

    void setContour2DNumContours(const DataSetId& id, int numContours);

    void setContour2DContourList(const DataSetId& id, const std::vector<double>& contours);

    void setContour2DColorMode(const DataSetId& id,
                               Contour2D::ColorMode mode,
                               const char *name, double range[2] = NULL);

    void setContour2DColorMode(const DataSetId& id,
                               Contour2D::ColorMode mode,
                               DataSet::DataAttributeType type,
                               const char *name, double range[2] = NULL);

    // 3D Contour (isosurface) plots

    bool addContour3D(const DataSetId& id, int numContours);

    bool addContour3D(const DataSetId& id, const std::vector<double>& contours);

    void setContour3DContourField(const DataSetId& id, const char *fieldName);

    void setContour3DNumContours(const DataSetId& id, int numContours);

    void setContour3DContourList(const DataSetId& id, const std::vector<double>& contours);

    void setContour3DColorMode(const DataSetId& id,
                               Contour3D::ColorMode mode,
                               const char *name, double range[2] = NULL);

    void setContour3DColorMode(const DataSetId& id,
                               Contour3D::ColorMode mode,
                               DataSet::DataAttributeType type,
                               const char *name, double range[2] = NULL);

    // Cutplanes

    void setCutplaneCloudStyle(const DataSetId& id,
                               Cutplane::CloudStyle style);

    void setCutplaneOutlineVisibility(const DataSetId& id, bool state);

    void setCutplaneSliceVisibility(const DataSetId& id, Axis axis, bool state);

    void setCutplaneColorMode(const DataSetId& id,
                              Cutplane::ColorMode mode,
                              const char *name, double range[2] = NULL);

    void setCutplaneColorMode(const DataSetId& id,
                              Cutplane::ColorMode mode,
                              DataSet::DataAttributeType type,
                              const char *name, double range[2] = NULL);

    // Cylinders

    bool addCylinder(const DataSetId& id, double radius, double height, bool cap, bool flipNormals = false);

    void setCylinderCapping(const DataSetId& id, bool state);

    void setCylinderResolution(const DataSetId& id, int res);

    // Disks

    bool addDisk(const DataSetId& id, double innerRadius, double outerRadius, bool flipNormals = false);

    void setDiskResolution(const DataSetId& id, int resRadial, int resCircum);

    // Glyphs

    bool addGlyphs(const DataSetId& id, Glyphs::GlyphShape shape);

    void setGlyphsShape(const DataSetId& id, Glyphs::GlyphShape shape);

    void setGlyphsOrientMode(const DataSetId& id, bool state, const char *name);

    void setGlyphsQuality(const DataSetId& id, double quality);

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

    void setGlyphsMaximumNumberOfGlyphs(const DataSetId& id, int max,
                                        bool random = true,
                                        int offset = 0, int ratio = 1);

    // Groups

    bool addGroup(const DataSetId& id, const std::vector<Group::NodeId>& nodeList);

    void addChildrenToGroup(const DataSetId& id, const std::vector<Group::NodeId>& nodeList);

    void removeChildrenFromGroup(const DataSetId& id, const std::vector<Group::NodeId>& nodeList);

    // Height maps

    bool addHeightMap(const DataSetId& id, int numContours, double heightScale);

    bool addHeightMap(const DataSetId& id, const std::vector<double>& contours, double heightScale);

    void setHeightMapHeightScale(const DataSetId& id, double scale);

    void setHeightMapCloudStyle(const DataSetId& id,
                                HeightMap::CloudStyle style);

    void setHeightMapNumContours(const DataSetId& id, int numContours);

    void setHeightMapContourList(const DataSetId& id, const std::vector<double>& contours);

    void setHeightMapContourSurfaceVisibility(const DataSetId& id, bool state);

    void setHeightMapContourLineVisibility(const DataSetId& id, bool state);

    void setHeightMapContourLineColorMapEnabled(const DataSetId& id, bool mode);

    void setHeightMapContourEdgeColor(const DataSetId& id, float color[3]);

    void setHeightMapContourEdgeWidth(const DataSetId& id, float edgeWidth);

    void setHeightMapColorMode(const DataSetId& id,
                               HeightMap::ColorMode mode,
                               const char *name, double range[2] = NULL);

    void setHeightMapColorMode(const DataSetId& id,
                               HeightMap::ColorMode mode,
                               DataSet::DataAttributeType type,
                               const char *name, double range[2] = NULL);

    // Images

    void setImageBackground(const DataSetId& id, bool state);

    void setImageBacking(const DataSetId& id, bool state);

    void setImageBorder(const DataSetId& id, bool state);

    void setImageExtents(const DataSetId& id, int extents[6]);

    void setImageLevel(const DataSetId& id, double level);

    void setImageWindow(const DataSetId& id, double window);

    void setImageZSlice(const DataSetId& id, int z);

    // ImageCutplanes

    void setImageCutplaneOutlineVisibility(const DataSetId& id, bool state);

    void setImageCutplaneSliceVisibility(const DataSetId& id, Axis axis, bool state);

    void setImageCutplaneLevel(const DataSetId& id, double level);

    void setImageCutplaneWindow(const DataSetId& id, double window);

    // Lines

    bool addLine(const DataSetId& id, double pt1[3], double pt2[3]);

    bool addLine(const DataSetId& id, std::vector<double> points);

    // Molecules

    void setMoleculeAtomQuality(const DataSetId& id, double quality);

    void setMoleculeBondQuality(const DataSetId& id, double quality);

    void setMoleculeAtomRadiusScale(const DataSetId& id, double scale);

    void setMoleculeBondRadiusScale(const DataSetId& id, double scale);

    void setMoleculeAtomScaling(const DataSetId& id, Molecule::AtomScaling scaling);

    void setMoleculeAtomVisibility(const DataSetId& id, bool state);

    void setMoleculeAtomLabelField(const DataSetId& id, const char *fieldName);

    void setMoleculeAtomLabelVisibility(const DataSetId& id, bool state);

    void setMoleculeBondVisibility(const DataSetId& id, bool state);

    void setMoleculeBondStyle(const DataSetId& id, Molecule::BondStyle style);

    void setMoleculeBondColorMode(const DataSetId& id, Molecule::BondColorMode mode);

    void setMoleculeBondColor(const DataSetId& id, float color[3]);

    void setMoleculeColorMode(const DataSetId& id,
                              Molecule::ColorMode mode,
                              const char *name, double range[2] = NULL);

    void setMoleculeColorMode(const DataSetId& id,
                              Molecule::ColorMode mode,
                              DataSet::DataAttributeType type,
                              const char *name, double range[2] = NULL);

    // Parallelepipeds

    bool addParallelepiped(const DataSetId& id, double vec1[3], double vec2[3], double vec3[3],
                           bool flipNormals = false);

    // PolyData meshes

    void setPolyDataCloudStyle(const DataSetId& id,
                               PolyData::CloudStyle style);

    void setPolyDataColorMode(const DataSetId& id,
                              PolyData::ColorMode mode,
                              const char *name, double range[2] = NULL);

    void setPolyDataColorMode(const DataSetId& id,
                              PolyData::ColorMode mode,
                              DataSet::DataAttributeType type,
                              const char *name, double range[2] = NULL);

    // N-sided Regular Polygons

    bool addPolygon(const DataSetId& id, int numSides, double center[3], double normal[3], double radius);

    // Color-mapped surfaces

    void setPseudoColorCloudStyle(const DataSetId& id,
                                  PseudoColor::CloudStyle style);

    void setPseudoColorColorMode(const DataSetId& id,
                                 PseudoColor::ColorMode mode,
                                 const char *name, double range[2] = NULL);

    void setPseudoColorColorMode(const DataSetId& id,
                                 PseudoColor::ColorMode mode,
                                 DataSet::DataAttributeType type,
                                 const char *name, double range[2] = NULL);

    // Spheres

    bool addSphere(const DataSetId& id, double center[3], double radius, bool flipNormals = false);

    void setSphereSection(const DataSetId& id,
                          double thetaStart, double thetaEnd,
                          double phiStart, double phiEnd);

    void setSphereResolution(const DataSetId& id, int thetaRes, int phiRes);

    // Streamlines

    void setStreamlinesColorMode(const DataSetId& id,
                                 Streamlines::ColorMode mode,
                                 const char *name, double range[2] = NULL);

    void setStreamlinesColorMode(const DataSetId& id,
                                 Streamlines::ColorMode mode,
                                 DataSet::DataAttributeType type,
                                 const char *name, double range[2] = NULL);

    void setStreamlinesLength(const DataSetId& id, double length);

    void setStreamlinesNumberOfSeedPoints(const DataSetId& id, int numPoints);

    void setStreamlinesSeedColor(const DataSetId& id, float color[3]);

    void setStreamlinesSeedPointSize(const DataSetId& id, float size);

    void setStreamlinesSeedToMeshPoints(const DataSetId& id,
                                        int maxPoints = 500);

    void setStreamlinesSeedToFilledMesh(const DataSetId& id, int numPoints);

    bool setStreamlinesSeedToMeshPoints(const DataSetId& id,
                                        char *data, size_t nbytes,
                                        int maxPoints = 500);

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

    void setStreamlinesSeedVisibility(const DataSetId& id, bool state);

    void setStreamlinesTerminalSpeed(const DataSetId& id, double speed);

    void setStreamlinesTypeToLines(const DataSetId& id);

    void setStreamlinesTypeToTubes(const DataSetId& id, int numSides, double radius);

    void setStreamlinesTypeToRibbons(const DataSetId& id, double width, double angle);

    // Text3Ds

    bool addText3D(const DataSetId& id, const char *string,
                   const char *fontFamily, int fontSize,
                   bool bold = false, bool italic = false, bool shadow = false);

    void setText3DBold(const DataSetId& id, bool state);

    void setText3DFollowCamera(const DataSetId& id, bool state);

    void setText3DFont(const DataSetId& id, const char *fontFamily);

    void setText3DFontSize(const DataSetId& id, int size);

    void setText3DItalic(const DataSetId& id, bool state);

    void setText3DShadow(const DataSetId& id, bool state);

    void setText3DText(const DataSetId& id, const char *text);

    // Volumes

    void setVolumeBlendMode(const DataSetId& id, Volume::BlendMode mode);

    void setVolumeSampleDistance(const DataSetId& id, double distance);

    // Warps

    void setWarpCloudStyle(const DataSetId& id,
                           Warp::CloudStyle style);

    void setWarpColorMode(const DataSetId& id,
                          Warp::ColorMode mode,
                          const char *name, double range[2] = NULL);

    void setWarpColorMode(const DataSetId& id,
                          Warp::ColorMode mode,
                          DataSet::DataAttributeType type,
                          const char *name, double range[2] = NULL);


    void setWarpWarpScale(const DataSetId& id, double scale);

private:
    typedef std::tr1::unordered_map<DataSetId, DataSet *> DataSetHashmap;
    typedef std::tr1::unordered_map<FieldId, double *> FieldRangeHashmap;
    typedef std::tr1::unordered_map<ColorMapId, ColorMap *> ColorMapHashmap;

    typedef std::tr1::unordered_map<DataSetId, Arc *> ArcHashmap;
    typedef std::tr1::unordered_map<DataSetId, Arrow *> ArrowHashmap;
    typedef std::tr1::unordered_map<DataSetId, Box *> BoxHashmap;
    typedef std::tr1::unordered_map<DataSetId, Cone *> ConeHashmap;
    typedef std::tr1::unordered_map<DataSetId, Contour2D *> Contour2DHashmap;
    typedef std::tr1::unordered_map<DataSetId, Contour3D *> Contour3DHashmap;
    typedef std::tr1::unordered_map<DataSetId, Cutplane *> CutplaneHashmap;
    typedef std::tr1::unordered_map<DataSetId, Cylinder *> CylinderHashmap;
    typedef std::tr1::unordered_map<DataSetId, Disk *> DiskHashmap;
    typedef std::tr1::unordered_map<DataSetId, Glyphs *> GlyphsHashmap;
    typedef std::tr1::unordered_map<DataSetId, Group *> GroupHashmap;
    typedef std::tr1::unordered_map<DataSetId, HeightMap *> HeightMapHashmap;
    typedef std::tr1::unordered_map<DataSetId, Image *> ImageHashmap;
    typedef std::tr1::unordered_map<DataSetId, ImageCutplane *> ImageCutplaneHashmap;
    typedef std::tr1::unordered_map<DataSetId, LIC *> LICHashmap;
    typedef std::tr1::unordered_map<DataSetId, Line *> LineHashmap;
    typedef std::tr1::unordered_map<DataSetId, Molecule *> MoleculeHashmap;
    typedef std::tr1::unordered_map<DataSetId, Outline *> OutlineHashmap;
    typedef std::tr1::unordered_map<DataSetId, Parallelepiped *> ParallelepipedHashmap;
    typedef std::tr1::unordered_map<DataSetId, PolyData *> PolyDataHashmap;
    typedef std::tr1::unordered_map<DataSetId, Polygon *> PolygonHashmap;
    typedef std::tr1::unordered_map<DataSetId, PseudoColor *> PseudoColorHashmap;
    typedef std::tr1::unordered_map<DataSetId, Sphere *> SphereHashmap;
    typedef std::tr1::unordered_map<DataSetId, Streamlines *> StreamlinesHashmap;
    typedef std::tr1::unordered_map<DataSetId, Text3D *> Text3DHashmap;
    typedef std::tr1::unordered_map<DataSetId, Volume *> VolumeHashmap;
    typedef std::tr1::unordered_map<DataSetId, Warp *> WarpHashmap;

    void _setCameraZoomRegionPixels(int x, int y, int width, int height);

    void _setCameraZoomRegion(double x, double y, double width, double height);

    //static void printCameraInfo(vtkCamera *camera);

    static void setCameraFromMatrix(vtkCamera *camera, vtkMatrix4x4 &mat);

    static void mergeBounds(double *boundsDest, const double *bounds1, const double *bounds2);

    template<class T>
    std::tr1::unordered_map<DataSetId, T *>& getGraphicsObjectHashmap();

    void setObjectAspects(double aspectRatio);

    void collectBounds(double *bounds, bool onlyVisible);

    void collectUnscaledBounds(double *bounds, bool onlyVisible);

    void collectDataRanges();

    void collectDataRanges(double *range, const char *name,
                           DataSet::DataAttributeType type,
                           int component, bool onlyVisible);

    void clearFieldRanges();

    void clearUserFieldRanges();

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

    void initCamera(bool initCameraMode = false);

    void sceneBoundsChanged();

    void initOrientationMarkers();

    void initAxes();

    void resetAxes(double bounds[6] = NULL);

    void setAxesBounds(double bounds[6] = NULL);

    void setAxesRanges();

    void computeAxesScale();

    void setCameraClippingPlanes();

    bool _needsRedraw;
    bool _needsAxesReset;
    bool _needsCameraClippingRangeReset;
    bool _needsCameraReset;

    int _windowWidth, _windowHeight;
    CameraMode _cameraMode;
    Aspect _cameraAspect;
    double _imgWorldOrigin[2];
    double _imgWorldDims[2];
    double _imgWindowWorldDims[2];
    double _userImgWorldOrigin[2];
    double _userImgWorldDims[2];
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
    FieldRangeHashmap _scalarFieldDataRange;
    FieldRangeHashmap _vectorFieldDataRange;
    FieldRangeHashmap _vectorCompFieldDataRange[3];

    FieldRangeHashmap _userScalarPointDataRange;
    FieldRangeHashmap _userVectorPointDataRange;
    FieldRangeHashmap _userVectorCompPointDataRange[3];
    FieldRangeHashmap _userScalarCellDataRange;
    FieldRangeHashmap _userVectorCellDataRange;
    FieldRangeHashmap _userVectorCompCellDataRange[3];
    FieldRangeHashmap _userScalarFieldDataRange;
    FieldRangeHashmap _userVectorFieldDataRange;
    FieldRangeHashmap _userVectorCompFieldDataRange[3];

    bool _axesAutoBounds[3];
    double _axesUserBounds[6];
    AxisRangeMode _axesRangeMode[3];
    double _axesScale[3];

    ColorMapHashmap _colorMaps;
    DataSetHashmap _dataSets;
    ArcHashmap _arcs;
    ArrowHashmap _arrows;
    BoxHashmap _boxes;
    ConeHashmap _cones;
    Contour2DHashmap _contour2Ds;
    Contour3DHashmap _contour3Ds;
    CutplaneHashmap _cutplanes;
    CylinderHashmap _cylinders;
    DiskHashmap _disks;
    GlyphsHashmap _glyphs;
    GroupHashmap _groups;
    HeightMapHashmap _heightMaps;
    ImageHashmap _images;
    ImageCutplaneHashmap _imageCutplanes;
    LICHashmap _lics;
    LineHashmap _lines;
    MoleculeHashmap _molecules;
    OutlineHashmap _outlines;
    ParallelepipedHashmap _parallelepipeds;
    PolyDataHashmap _polyDatas;
    PolygonHashmap _polygons;
    PseudoColorHashmap _pseudoColors;
    SphereHashmap _spheres;
    StreamlinesHashmap _streamlines;
    Text3DHashmap _text3Ds;
    VolumeHashmap _volumes;
    WarpHashmap _warps;

    vtkSmartPointer<vtkPlane> _cameraClipPlanes[4];
    vtkSmartPointer<vtkPlane> _userClipPlanes[6];
    vtkSmartPointer<vtkPlaneCollection> _activeClipPlanes;
#ifdef USE_CUSTOM_AXES
    vtkSmartPointer<vtkRpCubeAxesActor> _cubeAxesActor;
#else
    vtkSmartPointer<vtkCubeAxesActor> _cubeAxesActor;
#endif
    vtkSmartPointer<vtkAxesActor> _axesActor;
    vtkSmartPointer<vtkAnnotatedCubeActor> _annotatedCubeActor;
    vtkSmartPointer<vtkScalarBarActor> _scalarBarActor;
    vtkSmartPointer<vtkRenderer> _renderer;
    vtkSmartPointer<vtkRenderer> _legendRenderer;
    vtkSmartPointer<vtkRenderWindow> _renderWindow;
    vtkSmartPointer<vtkRenderWindow> _legendRenderWindow;
};

}

#endif
