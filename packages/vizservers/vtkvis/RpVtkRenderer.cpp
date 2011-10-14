/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cfloat>
#include <cstring>
#include <cassert>
#include <cmath>

#include <GL/gl.h>

#ifdef WANT_TRACE
#include <sys/time.h>
#endif

#include <vtkMath.h>
#include <vtkCamera.h>
#include <vtkLight.h>
#include <vtkCoordinate.h>
#include <vtkTransform.h>
#include <vtkCharArray.h>
#include <vtkAxisActor2D.h>
#include <vtkCubeAxesActor.h>
#ifdef USE_CUSTOM_AXES
#include "vtkRpCubeAxesActor2D.h"
#else
#include <vtkCubeAxesActor2D.h>
#endif
#include <vtkDataSetReader.h>
#include <vtkDataSetMapper.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkVersion.h>

#include "RpVtkRenderer.h"
#include "RpVtkRendererGraphicsObjs.h"
#include "RpMath.h"
#include "ColorMap.h"
#include "Trace.h"

#define ELAPSED_TIME(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3f) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3f + (float)((t2).tv_usec - (t1).tv_usec)/1.0e+3f)

using namespace Rappture::VtkVis;

Renderer::Renderer() :
    _needsRedraw(true),
    _windowWidth(500),
    _windowHeight(500),
    _imgCameraPlane(PLANE_XY),
    _imgCameraOffset(0),
    _cameraZoomRatio(1),
    _useCumulativeRange(true),
    _cumulativeRangeOnlyVisible(false),
    _cameraMode(PERSPECTIVE)
{
    _bgColor[0] = 0;
    _bgColor[1] = 0;
    _bgColor[2] = 0;
    _cameraPan[0] = 0;
    _cameraPan[1] = 0;
    _cameraOrientation[0] = 1.0;
    _cameraOrientation[1] = 0.0;
    _cameraOrientation[2] = 0.0;
    _cameraOrientation[3] = 0.0;
    // clipping planes to prevent overdrawing axes
    _activeClipPlanes = vtkSmartPointer<vtkPlaneCollection>::New();
    // bottom
    _cameraClipPlanes[0] = vtkSmartPointer<vtkPlane>::New();
    _cameraClipPlanes[0]->SetNormal(0, 1, 0);
    _cameraClipPlanes[0]->SetOrigin(0, 0, 0);
    if (_cameraMode == IMAGE)
        _activeClipPlanes->AddItem(_cameraClipPlanes[0]);
    // left
    _cameraClipPlanes[1] = vtkSmartPointer<vtkPlane>::New();
    _cameraClipPlanes[1]->SetNormal(1, 0, 0);
    _cameraClipPlanes[1]->SetOrigin(0, 0, 0);
    if (_cameraMode == IMAGE)
        _activeClipPlanes->AddItem(_cameraClipPlanes[1]);
    // top
    _cameraClipPlanes[2] = vtkSmartPointer<vtkPlane>::New();
    _cameraClipPlanes[2]->SetNormal(0, -1, 0);
    _cameraClipPlanes[2]->SetOrigin(0, 1, 0);
    if (_cameraMode == IMAGE)
        _activeClipPlanes->AddItem(_cameraClipPlanes[2]);
    // right
    _cameraClipPlanes[3] = vtkSmartPointer<vtkPlane>::New();
    _cameraClipPlanes[3]->SetNormal(-1, 0, 0);
    _cameraClipPlanes[3]->SetOrigin(1, 0, 0);
    if (_cameraMode == IMAGE)
        _activeClipPlanes->AddItem(_cameraClipPlanes[3]);
    _renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkLight> headlight = vtkSmartPointer<vtkLight>::New();
    headlight->SetLightTypeToHeadlight();
    headlight->PositionalOff();
    //headlight->SetAmbientColor(1, 1, 1);
    _renderer->SetAmbient(.2, .2, .2);
    _renderer->AddLight(headlight);
    vtkSmartPointer<vtkLight> skylight = vtkSmartPointer<vtkLight>::New();
    skylight->SetLightTypeToCameraLight();
    skylight->SetPosition(0, 1, 0);
    skylight->SetFocalPoint(0, 0, 0);
    skylight->PositionalOff();
    //skylight->SetAmbientColor(1, 1, 1);
    _renderer->AddLight(skylight);
    _renderer->LightFollowCameraOn();
    _renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
#ifdef USE_OFFSCREEN_RENDERING
    _renderWindow->DoubleBufferOff();
    _renderWindow->OffScreenRenderingOn();
#else
    _renderWindow->SwapBuffersOff();
#endif
    _renderWindow->SetSize(_windowWidth, _windowHeight);
    // Next 2 options needed to support depth peeling
    _renderWindow->SetAlphaBitPlanes(1);
    _renderWindow->SetMultiSamples(0);
    _renderer->SetMaximumNumberOfPeels(100);
    _renderer->SetUseDepthPeeling(1);
    _renderWindow->AddRenderer(_renderer);
    setViewAngle(_windowHeight);
    initAxes();
    initCamera();
    addColorMap("default", ColorMap::getDefault());
    addColorMap("grayDefault", ColorMap::getGrayDefault());
    addColorMap("volumeDefault", ColorMap::getVolumeDefault());
    addColorMap("elementDefault", ColorMap::getElementDefault());
}

Renderer::~Renderer()
{
    TRACE("Enter");
    TRACE("Deleting Contour2Ds");
    for (Contour2DHashmap::iterator itr = _contour2Ds.begin();
         itr != _contour2Ds.end(); ++itr) {
        delete itr->second;
    }
    _contour2Ds.clear();
    TRACE("Deleting Contour3Ds");
    for (Contour3DHashmap::iterator itr = _contour3Ds.begin();
         itr != _contour3Ds.end(); ++itr) {
        delete itr->second;
    }
    _contour3Ds.clear();
    TRACE("Deleting Cutplanes");
    for (CutplaneHashmap::iterator itr = _cutplanes.begin();
         itr != _cutplanes.end(); ++itr) {
        delete itr->second;
    }
    _cutplanes.clear();
    TRACE("Deleting Glyphs");
    for (GlyphsHashmap::iterator itr = _glyphs.begin();
         itr != _glyphs.end(); ++itr) {
        delete itr->second;
    }
    _glyphs.clear();
    TRACE("Deleting HeightMaps");
    for (HeightMapHashmap::iterator itr = _heightMaps.begin();
         itr != _heightMaps.end(); ++itr) {
        delete itr->second;
    }
    _heightMaps.clear();
    TRACE("Deleting LICs");
    for (LICHashmap::iterator itr = _lics.begin();
         itr != _lics.end(); ++itr) {
        delete itr->second;
    }
    _lics.clear();
    TRACE("Deleting Molecules");
    for (MoleculeHashmap::iterator itr = _molecules.begin();
         itr != _molecules.end(); ++itr) {
        delete itr->second;
    }
    _molecules.clear();
    TRACE("Deleting PolyDatas");
    for (PolyDataHashmap::iterator itr = _polyDatas.begin();
         itr != _polyDatas.end(); ++itr) {
        delete itr->second;
    }
    _polyDatas.clear();
    TRACE("Deleting PseudoColors");
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
         itr != _pseudoColors.end(); ++itr) {
        delete itr->second;
    }
    _pseudoColors.clear();
    TRACE("Deleting Streamlines");
    for (StreamlinesHashmap::iterator itr = _streamlines.begin();
         itr != _streamlines.end(); ++itr) {
        delete itr->second;
    }
    _streamlines.clear();
    TRACE("Deleting Volumes");
    for (VolumeHashmap::iterator itr = _volumes.begin();
         itr != _volumes.end(); ++itr) {
        delete itr->second;
    }
    _volumes.clear();
    TRACE("Deleting ColorMaps");
    // Delete color maps and data sets last in case references still
    // exist
    for (ColorMapHashmap::iterator itr = _colorMaps.begin();
         itr != _colorMaps.end(); ++itr) {
        delete itr->second;
    }
    _colorMaps.clear();
    TRACE("Deleting DataSets");
    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        delete itr->second;
    }
    _dataSets.clear();

    clearFieldRanges();

    TRACE("Leave");
}

/**
 * \brief Add a DataSet to this Renderer
 *
 * This just adds the DataSet to the Renderer's list of data sets.
 * In order to render the data, a graphics object using the data
 * set must be added to the Renderer.
 */
void Renderer::addDataSet(const DataSetId& id)
{
    if (getDataSet(id) != NULL) {
        WARN("Replacing existing DataSet %s", id.c_str());
        deleteDataSet(id);
    }
    _dataSets[id] = new DataSet(id);
}

/**
 * \brief Remove the specified DataSet and associated rendering objects
 *
 * The underlying DataSet and any associated graphics 
 * objects are deleted, freeing the memory used.
 */
void Renderer::deleteDataSet(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
        doAll = true;
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return;
    }

    do {
        TRACE("Deleting dataset %s", itr->second->getName().c_str());

        deleteGraphicsObject<Contour2D>(itr->second->getName());
        deleteGraphicsObject<Contour3D>(itr->second->getName());
        deleteGraphicsObject<Cutplane>(itr->second->getName());
        deleteGraphicsObject<Glyphs>(itr->second->getName());
        deleteGraphicsObject<HeightMap>(itr->second->getName());
        deleteGraphicsObject<LIC>(itr->second->getName());
        deleteGraphicsObject<Molecule>(itr->second->getName());
        deleteGraphicsObject<PolyData>(itr->second->getName());
        deleteGraphicsObject<PseudoColor>(itr->second->getName());
        deleteGraphicsObject<Streamlines>(itr->second->getName());
        deleteGraphicsObject<Volume>(itr->second->getName());
 
        if (itr->second->getProp() != NULL) {
            _renderer->RemoveViewProp(itr->second->getProp());
        }

        TRACE("After deleting graphics objects");

        delete itr->second;
        itr = _dataSets.erase(itr);
    } while (doAll && itr != _dataSets.end());

    // Update cumulative data range
    initFieldRanges();
    updateFieldRanges();

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Get a list of DataSets this Renderer knows about
 */
void Renderer::getDataSetNames(std::vector<std::string>& names)
{
    names.clear();
    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        names.push_back(itr->second->getName());
    }
}

/**
 * \brief Find the DataSet for the given DataSetId key
 *
 * \return A pointer to the DataSet, or NULL if not found
 */
DataSet *Renderer::getDataSet(const DataSetId& id)
{
    DataSetHashmap::iterator itr = _dataSets.find(id);
    if (itr == _dataSets.end()) {
#ifdef DEBUG
        TRACE("DataSet not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief (Re-)load the data for the specified DataSet key from a file
 */
bool Renderer::setDataFile(const DataSetId& id, const char *filename)
{
    DataSet *ds = getDataSet(id);
    if (ds) {
        bool ret = ds->setDataFile(filename);
        initFieldRanges();
        updateFieldRanges();
        _needsRedraw = true;
        return ret;
    } else
        return false;
}

/**
 * \brief (Re-)load the data for the specified DataSet key from a memory buffer
 */
bool Renderer::setData(const DataSetId& id, char *data, int nbytes)
{
    DataSet *ds = getDataSet(id);
    if (ds) {
        bool ret = ds->setData(data, nbytes);
        initFieldRanges();
        updateFieldRanges();
        _needsRedraw = true;
        return ret;
    } else
        return false;
}

/**
 * \brief Set the active scalar field array by name for a DataSet
 */
bool Renderer::setDataSetActiveScalars(const DataSetId& id, const char *scalarName)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
        doAll = true;
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("DataSet not found: %s", id.c_str());
        return false;
    }

    bool ret = true;
    bool needRangeUpdate = false;
    do {
        if (strcmp(itr->second->getActiveScalarsName(), scalarName) != 0) {
            if (!itr->second->setActiveScalars(scalarName)) {
                ret = false;
            } else {
                needRangeUpdate = true;
            }
        }
    } while (doAll && ++itr != _dataSets.end());

    if (needRangeUpdate) {
         updateFieldRanges();
        _needsRedraw = true;
    } 

    return ret;
}

/**
 * \brief Set the active vector field array by name for a DataSet
 */
bool Renderer::setDataSetActiveVectors(const DataSetId& id, const char *vectorName)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
        doAll = true;
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("DataSet not found: %s", id.c_str());
        return false;
    }

    bool ret = true;
    bool needRangeUpdate = false;
    do {
        if (strcmp(itr->second->getActiveVectorsName(), vectorName) != 0) {
            if (!itr->second->setActiveVectors(vectorName)) {
                ret = false;
            } else {
                needRangeUpdate = true;
            }
        }
    } while (doAll && ++itr != _dataSets.end());

    if (needRangeUpdate) {
        updateFieldRanges();
        _needsRedraw = true;
    } 

    return ret;
}

/**
 * \brief Control whether the cumulative data range of datasets is used for 
 * colormapping and contours
 */
void Renderer::setUseCumulativeDataRange(bool state, bool onlyVisible)
{
    if (state != _useCumulativeRange) {
        _useCumulativeRange = state;
        _cumulativeRangeOnlyVisible = onlyVisible;
        updateFieldRanges();
        _needsRedraw = true;
    }
}

void Renderer::resetAxes(double bounds[6])
{
    TRACE("Resetting axes");
    if (_cubeAxesActor == NULL ||
        _cubeAxesActor2D == NULL) {
        initAxes();
    }
    if (_cameraMode == IMAGE) {
        if (_renderer->HasViewProp(_cubeAxesActor)) {
            TRACE("Removing 3D axes");
            _renderer->RemoveViewProp(_cubeAxesActor);
        }
        if (!_renderer->HasViewProp(_cubeAxesActor2D)) {
            TRACE("Adding 2D axes");
            _renderer->AddViewProp(_cubeAxesActor2D);
        }
    } else {
        if (_renderer->HasViewProp(_cubeAxesActor2D)) {
            TRACE("Removing 2D axes");
            _renderer->RemoveViewProp(_cubeAxesActor2D);
        }
        if (!_renderer->HasViewProp(_cubeAxesActor)) {
            TRACE("Adding 3D axes");
            _renderer->AddViewProp(_cubeAxesActor);
        }
        if (bounds == NULL) {
            double newBounds[6];
            collectBounds(newBounds, false);
#if ((VTK_MAJOR_VERSION > 5) || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION >= 8))
            _cubeAxesActor->SetXAxisRange(newBounds[0], newBounds[1]);
            _cubeAxesActor->SetYAxisRange(newBounds[2], newBounds[3]);
            _cubeAxesActor->SetZAxisRange(newBounds[4], newBounds[5]);
#endif
            _cubeAxesActor->SetBounds(newBounds);
            TRACE("Bounds (computed): %g %g %g %g %g %g",
                  newBounds[0],
                  newBounds[1],
                  newBounds[2],
                  newBounds[3],
                  newBounds[4],
                  newBounds[5]);
        } else {
#if ((VTK_MAJOR_VERSION > 5) || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION >= 8))
            _cubeAxesActor->SetXAxisRange(bounds[0], bounds[1]);
            _cubeAxesActor->SetYAxisRange(bounds[2], bounds[3]);
            _cubeAxesActor->SetZAxisRange(bounds[4], bounds[5]);
#endif
            _cubeAxesActor->SetBounds(bounds);
            TRACE("Bounds (supplied): %g %g %g %g %g %g",
                  bounds[0],
                  bounds[1],
                  bounds[2],
                  bounds[3],
                  bounds[4],
                  bounds[5]);
        }
    }
}

/**
 * \brief Set inital properties on the 2D Axes
 */
void Renderer::initAxes()
{
    TRACE("Initializing axes");
    if (_cubeAxesActor == NULL)
        _cubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
    _cubeAxesActor->SetCamera(_renderer->GetActiveCamera());
    _cubeAxesActor->GetProperty()->LightingOff();
    // Don't offset labels at origin
    _cubeAxesActor->SetCornerOffset(0);
    _cubeAxesActor->SetFlyModeToStaticTriad();

#ifdef USE_CUSTOM_AXES
    if (_cubeAxesActor2D == NULL)
        _cubeAxesActor2D = vtkSmartPointer<vtkRpCubeAxesActor2D>::New();
#else
    if (_cubeAxesActor2D == NULL)
        _cubeAxesActor2D = vtkSmartPointer<vtkCubeAxesActor2D>::New();
#endif

    _cubeAxesActor2D->SetCamera(_renderer->GetActiveCamera());
    _cubeAxesActor2D->ZAxisVisibilityOff();
    _cubeAxesActor2D->SetCornerOffset(0);
    _cubeAxesActor2D->SetFlyModeToClosestTriad();

    _cubeAxesActor2D->ScalingOff();
    //_cubeAxesActor2D->SetShowActualBounds(0);
    _cubeAxesActor2D->SetFontFactor(1.25);
    // Use "nice" range and number of ticks/labels
    _cubeAxesActor2D->GetXAxisActor2D()->AdjustLabelsOn();
    _cubeAxesActor2D->GetYAxisActor2D()->AdjustLabelsOn();
    _cubeAxesActor2D->GetZAxisActor2D()->AdjustLabelsOn();

#ifdef USE_CUSTOM_AXES
    _cubeAxesActor2D->SetAxisTitleTextProperty(NULL);
    _cubeAxesActor2D->SetAxisLabelTextProperty(NULL);
    //_cubeAxesActor2D->GetXAxisActor2D()->SizeFontRelativeToAxisOn();
    _cubeAxesActor2D->GetXAxisActor2D()->GetTitleTextProperty()->BoldOn();
    _cubeAxesActor2D->GetXAxisActor2D()->GetTitleTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetXAxisActor2D()->GetTitleTextProperty()->ShadowOn();
    _cubeAxesActor2D->GetXAxisActor2D()->GetLabelTextProperty()->BoldOff();
    _cubeAxesActor2D->GetXAxisActor2D()->GetLabelTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetXAxisActor2D()->GetLabelTextProperty()->ShadowOff();

    //_cubeAxesActor2D->GetYAxisActor2D()->SizeFontRelativeToAxisOn();
    _cubeAxesActor2D->GetYAxisActor2D()->GetTitleTextProperty()->BoldOn();
    _cubeAxesActor2D->GetYAxisActor2D()->GetTitleTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetYAxisActor2D()->GetTitleTextProperty()->ShadowOn();
    _cubeAxesActor2D->GetYAxisActor2D()->GetLabelTextProperty()->BoldOff();
    _cubeAxesActor2D->GetYAxisActor2D()->GetLabelTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetYAxisActor2D()->GetLabelTextProperty()->ShadowOff();

    //_cubeAxesActor2D->GetZAxisActor2D()->SizeFontRelativeToAxisOn();
    _cubeAxesActor2D->GetZAxisActor2D()->GetTitleTextProperty()->BoldOn();
    _cubeAxesActor2D->GetZAxisActor2D()->GetTitleTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetZAxisActor2D()->GetTitleTextProperty()->ShadowOn();
    _cubeAxesActor2D->GetZAxisActor2D()->GetLabelTextProperty()->BoldOff();
    _cubeAxesActor2D->GetZAxisActor2D()->GetLabelTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetZAxisActor2D()->GetLabelTextProperty()->ShadowOff();
#else
    _cubeAxesActor2D->GetAxisTitleTextProperty()->BoldOn();
    _cubeAxesActor2D->GetAxisTitleTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetAxisTitleTextProperty()->ShadowOn();
    _cubeAxesActor2D->GetAxisLabelTextProperty()->BoldOff();
    _cubeAxesActor2D->GetAxisLabelTextProperty()->ItalicOff();
    _cubeAxesActor2D->GetAxisLabelTextProperty()->ShadowOff();
#endif

    if (_cameraMode == IMAGE) {
        if (!_renderer->HasViewProp(_cubeAxesActor2D))
            _renderer->AddViewProp(_cubeAxesActor2D);
    } else {
        if (!_renderer->HasViewProp(_cubeAxesActor))
            _renderer->AddViewProp(_cubeAxesActor);
    }
}

/**
 * \brief Set Fly mode of axes
 */
void Renderer::setAxesFlyMode(AxesFlyMode mode)
{
    if (_cubeAxesActor == NULL)
        initAxes();
    switch (mode) {
    case FLY_STATIC_EDGES:
        _cubeAxesActor->SetFlyModeToStaticEdges();
        break;
    case FLY_STATIC_TRIAD:
        _cubeAxesActor->SetFlyModeToStaticTriad();
        break;
    case FLY_OUTER_EDGES:
        _cubeAxesActor->SetFlyModeToOuterEdges();
        break;
    case FLY_FURTHEST_TRIAD:
        _cubeAxesActor->SetFlyModeToFurthestTriad();
        break;
    case FLY_CLOSEST_TRIAD:
    default:
        _cubeAxesActor->SetFlyModeToClosestTriad();
        break;
    }
    _needsRedraw = true;
}

/**
 * \brief Set color of axes, ticks, labels, titles
 */
void Renderer::setAxesColor(double color[3])
{
    if (_cubeAxesActor != NULL) {
        _cubeAxesActor->GetProperty()->SetColor(color);
        _needsRedraw = true;
    }
    if (_cubeAxesActor2D != NULL) {
        _cubeAxesActor2D->GetProperty()->SetColor(color);
#ifdef USE_CUSTOM_AXES
        _cubeAxesActor2D->GetXAxisActor2D()->GetTitleTextProperty()->SetColor(color);
        _cubeAxesActor2D->GetXAxisActor2D()->GetLabelTextProperty()->SetColor(color);
        _cubeAxesActor2D->GetYAxisActor2D()->GetTitleTextProperty()->SetColor(color);
        _cubeAxesActor2D->GetYAxisActor2D()->GetLabelTextProperty()->SetColor(color);
        _cubeAxesActor2D->GetZAxisActor2D()->GetTitleTextProperty()->SetColor(color);
        _cubeAxesActor2D->GetZAxisActor2D()->GetLabelTextProperty()->SetColor(color);
#else
        _cubeAxesActor2D->GetAxisTitleTextProperty()->SetColor(color);
        _cubeAxesActor2D->GetAxisLabelTextProperty()->SetColor(color);
#endif
        _needsRedraw = true;
    }
}

/**
 * \brief Turn on/off rendering of all enabled axes
 */
void Renderer::setAxesVisibility(bool state)
{
    if (_cubeAxesActor != NULL) {
        _cubeAxesActor->SetVisibility((state ? 1 : 0));
        _needsRedraw = true;
    }
    if (_cubeAxesActor2D != NULL) {
        _cubeAxesActor2D->SetVisibility((state ? 1 : 0));
        _needsRedraw = true;
    }
}

/**
 * \brief Turn on/off rendering of all axes gridlines
 */
void Renderer::setAxesGridVisibility(bool state)
{
    setAxisGridVisibility(X_AXIS, state);
    setAxisGridVisibility(Y_AXIS, state);
    setAxisGridVisibility(Z_AXIS, state);
}

/**
 * \brief Turn on/off rendering of all axis labels
 */
void Renderer::setAxesLabelVisibility(bool state)
{
    setAxisLabelVisibility(X_AXIS, state);
    setAxisLabelVisibility(Y_AXIS, state);
    setAxisLabelVisibility(Z_AXIS, state);
}

/**
 * \brief Turn on/off rendering of all axis ticks
 */
void Renderer::setAxesTickVisibility(bool state)
{
    setAxisTickVisibility(X_AXIS, state);
    setAxisTickVisibility(Y_AXIS, state);
    setAxisTickVisibility(Z_AXIS, state);
}

/**
 * \brief Control position of ticks on 3D axes
 */
void Renderer::setAxesTickPosition(AxesTickPosition pos)
{
    if (_cubeAxesActor == NULL)
        return;

    switch (pos) {
    case TICKS_BOTH:
        _cubeAxesActor->SetTickLocationToBoth();
        break;
    case TICKS_OUTSIDE:
        _cubeAxesActor->SetTickLocationToOutside();
        break;
    case TICKS_INSIDE:
    default:
        _cubeAxesActor->SetTickLocationToInside();
        break;
    }
    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the specified axis
 */
void Renderer::setAxisVisibility(Axis axis, bool state)
{
    if (_cubeAxesActor != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor->SetXAxisVisibility((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _cubeAxesActor->SetYAxisVisibility((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _cubeAxesActor->SetZAxisVisibility((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
    if (_cubeAxesActor2D != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor2D->SetXAxisVisibility((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _cubeAxesActor2D->SetYAxisVisibility((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _cubeAxesActor2D->SetZAxisVisibility((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
}

/**
 * \brief Turn on/off rendering of single axis gridlines
 */
void Renderer::setAxisGridVisibility(Axis axis, bool state)
{
    if (_cubeAxesActor != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor->SetDrawXGridlines((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _cubeAxesActor->SetDrawYGridlines((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _cubeAxesActor->SetDrawZGridlines((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
}

/**
 * \brief Toggle label visibility for the specified axis
 */
void Renderer::setAxisLabelVisibility(Axis axis, bool state)
{
    if (_cubeAxesActor != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor->SetXAxisLabelVisibility((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _cubeAxesActor->SetYAxisLabelVisibility((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _cubeAxesActor->SetZAxisLabelVisibility((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
    if (_cubeAxesActor2D != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor2D->GetXAxisActor2D()->SetLabelVisibility((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _cubeAxesActor2D->GetYAxisActor2D()->SetLabelVisibility((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _cubeAxesActor2D->GetZAxisActor2D()->SetLabelVisibility((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
}

/**
 * \brief Toggle tick visibility for the specified axis
 */
void Renderer::setAxisTickVisibility(Axis axis, bool state)
{
    if (_cubeAxesActor != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor->SetXAxisTickVisibility((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _cubeAxesActor->SetYAxisTickVisibility((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _cubeAxesActor->SetZAxisTickVisibility((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
    if (_cubeAxesActor2D != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor2D->GetXAxisActor2D()->SetTickVisibility((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _cubeAxesActor2D->GetYAxisActor2D()->SetTickVisibility((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _cubeAxesActor2D->GetZAxisActor2D()->SetTickVisibility((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
}

/**
 * \brief Set title of the specified axis
 */
void Renderer::setAxisTitle(Axis axis, const char *title)
{
    if (_cubeAxesActor != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor->SetXTitle(title);
        } else if (axis == Y_AXIS) {
            _cubeAxesActor->SetYTitle(title);
        } else if (axis == Z_AXIS) {
            _cubeAxesActor->SetZTitle(title);
        }
        _needsRedraw = true;
    }
    if (_cubeAxesActor2D != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor2D->SetXLabel(title);
        } else if (axis == Y_AXIS) {
            _cubeAxesActor2D->SetYLabel(title);
        } else if (axis == Z_AXIS) {
            _cubeAxesActor2D->SetZLabel(title);
        }
        _needsRedraw = true;
    }
}

/**
 * \brief Set units of the specified axis
 */
void Renderer::setAxisUnits(Axis axis, const char *units)
{
    if (_cubeAxesActor != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor->SetXUnits(units);
        } else if (axis == Y_AXIS) {
            _cubeAxesActor->SetYUnits(units);
        } else if (axis == Z_AXIS) {
            _cubeAxesActor->SetZUnits(units);
        }
        _needsRedraw = true;
    }
#ifdef notdef
    if (_cubeAxesActor2D != NULL) {
        if (axis == X_AXIS) {
            _cubeAxesActor2D->SetXUnits(units);
        } else if (axis == Y_AXIS) {
            _cubeAxesActor2D->SetYUnits(units);
        } else if (axis == Z_AXIS) {
            _cubeAxesActor2D->SetZUnits(units);
        }
        _needsRedraw = true;
    }
#endif
}

/**
 * \brief Notify graphics objects that color map has changed
 */
void Renderer::updateColorMap(ColorMap *cmap)
{
    for (Contour3DHashmap::iterator itr = _contour3Ds.begin();
         itr != _contour3Ds.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (CutplaneHashmap::iterator itr = _cutplanes.begin();
         itr != _cutplanes.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (GlyphsHashmap::iterator itr = _glyphs.begin();
         itr != _glyphs.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (HeightMapHashmap::iterator itr = _heightMaps.begin();
         itr != _heightMaps.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (LICHashmap::iterator itr = _lics.begin();
         itr != _lics.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (MoleculeHashmap::iterator itr = _molecules.begin();
         itr != _molecules.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
         itr != _pseudoColors.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (StreamlinesHashmap::iterator itr = _streamlines.begin();
         itr != _streamlines.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
    for (VolumeHashmap::iterator itr = _volumes.begin();
         itr != _volumes.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
}

/**
 * \brief Check if a ColorMap is in use by graphics objects
 */
bool Renderer::colorMapUsed(ColorMap *cmap)
{
    for (Contour3DHashmap::iterator itr = _contour3Ds.begin();
         itr != _contour3Ds.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (CutplaneHashmap::iterator itr = _cutplanes.begin();
         itr != _cutplanes.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (GlyphsHashmap::iterator itr = _glyphs.begin();
         itr != _glyphs.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (HeightMapHashmap::iterator itr = _heightMaps.begin();
         itr != _heightMaps.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (LICHashmap::iterator itr = _lics.begin();
         itr != _lics.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (MoleculeHashmap::iterator itr = _molecules.begin();
         itr != _molecules.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
         itr != _pseudoColors.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (StreamlinesHashmap::iterator itr = _streamlines.begin();
         itr != _streamlines.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    for (VolumeHashmap::iterator itr = _volumes.begin();
         itr != _volumes.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    return false;
}

/**
 * \brief Add/replace a ColorMap for use in the Renderer
 */
void Renderer::addColorMap(const ColorMapId& id, ColorMap *colorMap)
{
    if (colorMap != NULL) {
        colorMap->build();
        if (getColorMap(id) != NULL) {
            TRACE("Replacing existing ColorMap %s", id.c_str());
            // Copy to current colormap to avoid invalidating 
            // pointers in graphics objects using the color map
            *_colorMaps[id] = *colorMap;
            delete colorMap;
            // Notify graphics objects of change
            updateColorMap(_colorMaps[id]);
        } else
            _colorMaps[id] = colorMap;
    } else {
        ERROR("NULL ColorMap");
    }
}

/**
 * \brief Return the ColorMap associated with the colormap key given
 */
ColorMap *Renderer::getColorMap(const ColorMapId& id)
{
    ColorMapHashmap::iterator itr = _colorMaps.find(id);

    if (itr == _colorMaps.end())
        return NULL;
    else
        return itr->second;
}

/**
 * \brief Remove the colormap associated with the key given
 *
 * The underlying vtkLookupTable will be deleted if it is not referenced
 * by any other objects
 */
void Renderer::deleteColorMap(const ColorMapId& id)
{
    ColorMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _colorMaps.begin();
        doAll = true;
    } else {
        itr = _colorMaps.find(id);
    }

    if (itr == _colorMaps.end()) {
        ERROR("Unknown ColorMap %s", id.c_str());
        return;
    }

    do {
        if (itr->second->getName().compare("default") == 0 ||
            itr->second->getName().compare("grayDefault") == 0 ||
            itr->second->getName().compare("volumeDefault") == 0 ||
            itr->second->getName().compare("elementDefault") == 0) {
            if (id.compare("all") != 0) {
                WARN("Cannot delete a default color map");
            }
            continue;
        } else if (colorMapUsed(itr->second)) {
            WARN("Cannot delete color map '%s', it is in use", itr->second->getName().c_str());
            continue;
        }

        TRACE("Deleting ColorMap %s", itr->second->getName().c_str());

        delete itr->second;
        itr = _colorMaps.erase(itr);
    } while (doAll && itr != _colorMaps.end());
}

/**
 * \brief Render a labelled legend image for the given colormap
 *
 * The field is assumed to be the active scalar or vector field
 * based on the legendType.
 *
 * \param[in] id ColorMap name
 * \param[in] dataSetID DataSet name
 * \param[in] legendType scalar or vector field legend
 * \param[in,out] title If supplied, draw title ("#auto" means to
 * fill in field name and draw).  If blank, do not draw title.  
 * If title was blank or "#auto", will be filled with field name on
 * return
 * \param[in,out] range Data range to use in legend.  Set min > max to have
 * range computed, will be filled with valid min and max values
 * \param[in] width Pixel width of legend (aspect controls orientation)
 * \param[in] height Pixel height of legend (aspect controls orientation)
 * \param[in] numLabels Number of labels to render (includes min/max)
 * \param[in,out] imgData Pointer to array to fill with image bytes. Array
 * will be resized if needed.
 * \return The image is rendered into the supplied array, false is 
 * returned if the color map is not found
 */
bool Renderer::renderColorMap(const ColorMapId& id, 
                              const DataSetId& dataSetID,
                              Renderer::LegendType legendType,
                              std::string& title,
                              double range[2],
                              int width, int height,
                              int numLabels,
                              vtkUnsignedCharArray *imgData)
{
    DataSet *dataSet = NULL;
    if (dataSetID.compare("all") == 0) {
        if (_dataSets.empty()) {
            WARN("No DataSets exist, can't fill title or range");
            return renderColorMap(id, dataSetID, legendType,
                                  NULL,
                                  DataSet::POINT_DATA,
                                  title, range, width, height, numLabels, imgData);
        } else {
            dataSet = _dataSets.begin()->second;
        }
    } else {
        dataSet = getDataSet(dataSetID);
        if (dataSet == NULL) {
            ERROR("DataSet '%s' not found", dataSetID.c_str());
            return false;
        }
    }

    if (legendType == LEGEND_SCALAR) {
        return renderColorMap(id, dataSetID, legendType,
                              dataSet->getActiveScalarsName(),
                              dataSet->getActiveScalarsType(),
                              title, range, width, height, numLabels, imgData);
    } else {
        return renderColorMap(id, dataSetID, legendType,
                              dataSet->getActiveVectorsName(),
                              dataSet->getActiveVectorsType(),
                              title, range, width, height, numLabels, imgData);
    }
}

/**
 * \brief Render a labelled legend image for the given colormap
 *
 * The field is assumed to be point data, if the field is not found
 * as point data, cell data is used.
 *
 * \param[in] id ColorMap name
 * \param[in] dataSetID DataSet name
 * \param[in] legendType scalar or vector field legend
 * \param[in] fieldName Name of the field array this legend is for
 * \param[in,out] title If supplied, draw title ("#auto" means to
 * fill in field name and draw).  If blank, do not draw title.  
 * If title was blank or "#auto", will be filled with field name on
 * return
 * \param[in,out] range Data range to use in legend.  Set min > max to have
 * range computed, will be filled with valid min and max values
 * \param[in] width Pixel width of legend (aspect controls orientation)
 * \param[in] height Pixel height of legend (aspect controls orientation)
 * \param[in] numLabels Number of labels to render (includes min/max)
 * \param[in,out] imgData Pointer to array to fill with image bytes. Array
 * will be resized if needed.
 * \return The image is rendered into the supplied array, false is 
 * returned if the color map is not found
 */
bool Renderer::renderColorMap(const ColorMapId& id, 
                              const DataSetId& dataSetID,
                              Renderer::LegendType legendType,
                              const char *fieldName,
                              std::string& title,
                              double range[2],
                              int width, int height,
                              int numLabels,
                              vtkUnsignedCharArray *imgData)
{
    DataSet *dataSet = NULL;
    if (dataSetID.compare("all") == 0) {
        if (_dataSets.empty()) {
            WARN("No DataSets exist, can't fill title or range");
            return renderColorMap(id, dataSetID, legendType,
                                  NULL,
                                  DataSet::POINT_DATA,
                                  title, range, width, height, numLabels, imgData);
        } else {
            dataSet = _dataSets.begin()->second;
        }
    } else {
        dataSet = getDataSet(dataSetID);
        if (dataSet == NULL) {
            ERROR("DataSet '%s' not found", dataSetID.c_str());
            return false;
        }
    }

    DataSet::DataAttributeType attrType;
    int numComponents;

    dataSet->getFieldInfo(fieldName, &attrType, &numComponents);

    return renderColorMap(id, dataSetID, legendType,
                          fieldName,
                          attrType,
                          title, range, width, height, numLabels, imgData);
}

/**
 * \brief Render a labelled legend image for the given colormap
 *
 * \param[in] id ColorMap name
 * \param[in] dataSetID DataSet name
 * \param[in] legendType scalar or vector field legend
 * \param[in] fieldName Name of the field array this legend is for
 * \param[in] type DataAttributeType of the field
 * \param[in,out] title If supplied, draw title ("#auto" means to
 * fill in field name and draw).  If blank, do not draw title.  
 * If title was blank or "#auto", will be filled with field name on
 * return
 * \param[in,out] range Data range to use in legend.  Set min > max to have
 * range computed, will be filled with valid min and max values
 * \param[in] width Pixel width of legend (aspect controls orientation)
 * \param[in] height Pixel height of legend (aspect controls orientation)
 * \param[in] numLabels Number of labels to render (includes min/max)
 * \param[in,out] imgData Pointer to array to fill with image bytes. Array
 * will be resized if needed.
 * \return The image is rendered into the supplied array, false is 
 * returned if the color map is not found
 */
bool Renderer::renderColorMap(const ColorMapId& id, 
                              const DataSetId& dataSetID,
                              Renderer::LegendType legendType,
                              const char *fieldName,
                              DataSet::DataAttributeType type,
                              std::string& title,
                              double range[2],
                              int width, int height,
                              int numLabels,
                              vtkUnsignedCharArray *imgData)
{
    TRACE("Enter");
    ColorMap *colorMap = getColorMap(id);
    if (colorMap == NULL)
        return false;

    if (_legendRenderWindow == NULL) {
        _legendRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
        _legendRenderWindow->SetMultiSamples(0);
#ifdef USE_OFFSCREEN_RENDERING
        _legendRenderWindow->DoubleBufferOff();
        _legendRenderWindow->OffScreenRenderingOn();
#else
        _legendRenderWindow->DoubleBufferOn();
        _legendRenderWindow->SwapBuffersOff();
#endif
    }

    _legendRenderWindow->SetSize(width, height);

    if (_legendRenderer == NULL) {
        _legendRenderer = vtkSmartPointer<vtkRenderer>::New();
        _legendRenderWindow->AddRenderer(_legendRenderer);
    }
    _legendRenderer->SetBackground(_bgColor[0], _bgColor[1], _bgColor[2]);

    if (_scalarBarActor == NULL) {
        _scalarBarActor = vtkSmartPointer<vtkScalarBarActor>::New();
        _scalarBarActor->UseOpacityOn();
        _legendRenderer->AddViewProp(_scalarBarActor);
    }

    if (width > height) {
        _scalarBarActor->SetOrientationToHorizontal();
    } else {
        _scalarBarActor->SetOrientationToVertical();
    }

    // Set viewport-relative width/height/pos
    if (title.empty() && numLabels == 0) {
        _scalarBarActor->SetPosition(0, 0);
        if (width > height) {
            // horizontal
            _scalarBarActor->SetHeight(2.5); // VTK: floor(actorHeight * .4)
            _scalarBarActor->SetWidth(1);
        } else {
            // vertical
            _scalarBarActor->SetHeight((double)height/(.86*height)); // VTK: floor(actorHeight * .86)
            _scalarBarActor->SetWidth(((double)(width+5))/((double)width)); // VTK: actorWidth - 4 pixels
        }
    } else {
        if (width > height) {
            // horizontal
            _scalarBarActor->SetPosition(.075, .1);
            _scalarBarActor->SetHeight(0.8);
            _scalarBarActor->SetWidth(0.85);
        } else {
            // vertical
            _scalarBarActor->SetPosition(.1, .05);
            _scalarBarActor->SetHeight(0.9);
            _scalarBarActor->SetWidth(0.8);
        }
    }

    vtkSmartPointer<vtkLookupTable> lut = colorMap->getLookupTable();
    DataSet *dataSet = NULL;
    bool cumulative = _useCumulativeRange;
    if (dataSetID.compare("all") == 0) {
        if (_dataSets.empty()) {
            WARN("No DataSets exist, can't fill title or range");
        } else {
            dataSet = _dataSets.begin()->second;
        }
        cumulative = true;
    } else {
        dataSet = getDataSet(dataSetID);
        if (dataSet == NULL) {
            ERROR("DataSet '%s' not found", dataSetID.c_str());
            return false;
        }
    }

    bool drawTitle = false;
    if (!title.empty()) {
        drawTitle = true;
        if (title.compare("#auto") == 0) {
            title.clear();
        }
    }

    bool needRange = false;
    if (range[0] > range[1]) {
        range[0] = 0.0;
        range[1] = 1.0;
        needRange = true;
    } 

    switch (legendType) {
    case LEGEND_VECTOR_MAGNITUDE:
        if (needRange) {
            if (cumulative) {
                getCumulativeDataRange(range, fieldName, type, 3);
            } else if (dataSet != NULL) {
                dataSet->getDataRange(range, fieldName, type);
            }
        }

        lut->SetRange(range);

        if (title.empty() && dataSet != NULL) {
            if (fieldName != NULL) {
                title = fieldName;
                title.append("(mag)");
            }
        }
        break;
    case LEGEND_VECTOR_X:
        if (needRange) {
            if (cumulative) {
                getCumulativeDataRange(range, fieldName, type, 3, 0);
            } else if (dataSet != NULL) {
                dataSet->getDataRange(range, fieldName, type, 0);
            }
        }

        lut->SetRange(range);

        if (title.empty() && dataSet != NULL) {
            if (fieldName != NULL) {
                title = fieldName;
                title.append("(x)");
            }
        }
        break;
    case LEGEND_VECTOR_Y:
        if (needRange) {
            if (cumulative) {
                getCumulativeDataRange(range, fieldName, type, 3, 1);
            } else if (dataSet != NULL) {
                dataSet->getDataRange(range, fieldName, type, 1);
            }
        }

        lut->SetRange(range);

        if (title.empty() && dataSet != NULL) {
            if (fieldName != NULL) {
                title = fieldName;
                title.append("(y)");
            }
        }
        break;
    case LEGEND_VECTOR_Z:
        if (needRange) {
            if (cumulative) {
                getCumulativeDataRange(range, fieldName, type, 3, 2);
            } else if (dataSet != NULL) {
                dataSet->getDataRange(range, fieldName, type, 1);
            }
        }

        lut->SetRange(range);

        if (title.empty() && dataSet != NULL) {
            if (fieldName != NULL) {
                title = fieldName;
                title.append("(z)");
            }
        }
        break;
    case LEGEND_SCALAR:
    default:
        if (needRange) {
            if (cumulative) {
                getCumulativeDataRange(range, fieldName, type, 1);
            } else if (dataSet != NULL) {
                dataSet->getDataRange(range, fieldName, type);
            }
        }

        lut->SetRange(range);

        if (title.empty() && dataSet != NULL) {
            if (fieldName != NULL)
                title = fieldName;
        }
        break;
    }

    _scalarBarActor->SetLookupTable(lut);

    if (drawTitle) {
        _scalarBarActor->SetTitle(title.c_str());
    } else {
        _scalarBarActor->SetTitle("");
    }
    _scalarBarActor->GetTitleTextProperty()->ItalicOff();
    _scalarBarActor->SetNumberOfLabels(numLabels);
    _scalarBarActor->GetLabelTextProperty()->BoldOff();
    _scalarBarActor->GetLabelTextProperty()->ItalicOff();
    _scalarBarActor->GetLabelTextProperty()->ShadowOff();

    _legendRenderWindow->Render();

#ifdef RENDER_TARGA
    _legendRenderWindow->MakeCurrent();
    // Must clear previous errors first.
    while (glGetError() != GL_NO_ERROR){
        ;
    }
    int bytesPerPixel = TARGA_BYTES_PER_PIXEL;
    int size = bytesPerPixel * width * height;

    if (imgData->GetMaxId() + 1 != size)
    {
        imgData->SetNumberOfComponents(bytesPerPixel);
        imgData->SetNumberOfValues(size);
    }
    glDisable(GL_TEXTURE_2D);
    if (_legendRenderWindow->GetDoubleBuffer()) {
        glReadBuffer(static_cast<GLenum>(vtkOpenGLRenderWindow::SafeDownCast(_legendRenderWindow)->GetBackLeftBuffer()));
    } else {
        glReadBuffer(static_cast<GLenum>(vtkOpenGLRenderWindow::SafeDownCast(_legendRenderWindow)->GetFrontLeftBuffer()));
    }
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    if (bytesPerPixel == 4) {
        glReadPixels(0, 0, width, height, GL_BGRA,
                     GL_UNSIGNED_BYTE, imgData->GetPointer(0));
    } else {
        glReadPixels(0, 0, width, height, GL_BGR,
                     GL_UNSIGNED_BYTE, imgData->GetPointer(0));
    }
    if (glGetError() != GL_NO_ERROR) {
        ERROR("glReadPixels");
    }
#else
    _legendRenderWindow->GetPixelData(0, 0, width-1, height-1,
                                      !_legendRenderWindow->GetDoubleBuffer(),
                                      imgData);
#endif
    TRACE("Leave");
    return true;
}

/**
 * \brief Set camera FOV based on render window height
 * 
 * Computes a field-of-view angle based on some assumptions about
 * viewer's distance to screen and pixel density
 */
void Renderer::setViewAngle(int height)
{
    // Distance of eyes from screen in inches
    double d = 20.0;
    // Assume 72 ppi screen
    double h = (double)height / 72.0;

    double angle = vtkMath::DegreesFromRadians(2.0 * atan((h/2.0)/d));
    _renderer->GetActiveCamera()->SetViewAngle(angle);

    TRACE("Setting view angle: %g", angle);
}

/**
 * \brief Resize the render window (image size for renderings)
 */
void Renderer::setWindowSize(int width, int height)
{
    if (_windowWidth == width &&
        _windowHeight == height)
        return;

    //setViewAngle(height);

    // FIXME: Fix up panning on aspect change
#ifdef notdef
    if (_cameraPan[0] != 0.0) {
        _cameraPan[0] *= ((double)_windowWidth / width);
    }
    if (_cameraPan[1] != 0.0) {
        _cameraPan[1] *= ((double)_windowHeight / height);
    }
#endif
    _windowWidth = width;
    _windowHeight = height;
    _renderWindow->SetSize(_windowWidth, _windowHeight);
    if (_cameraMode == IMAGE) {
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0], _imgWorldDims[1]);
    }
    _needsRedraw = true;
}

/**
 * \brief Change the camera type: perspective, orthographic or image view
 *
 * Perspective mode is a normal 3D camera.
 *
 * Orthogrphic mode is parallel projection.
 *
 * Image mode is an orthographic camera with fixed axes and a clipping region 
 * around the plot area, use setCameraZoomRegion to control the displayed area
 *
 * \param[in] mode Enum specifying camera type
 */
void Renderer::setCameraMode(CameraMode mode)
{
    if (_cameraMode == mode) return;

    CameraMode origMode = _cameraMode;
    _cameraMode = mode;
    resetAxes();

    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    switch (mode) {
    case ORTHO: {
        TRACE("Set camera to Ortho mode");
        camera->ParallelProjectionOn();
        if (origMode == IMAGE) {
            resetCamera(true);
        }
        break;
    }
    case PERSPECTIVE: {
        TRACE("Set camera to Perspective mode");
        camera->ParallelProjectionOff();
        if (origMode == IMAGE) {
            resetCamera(true);
        }
        break;
    }
    case IMAGE: {
        camera->ParallelProjectionOn();
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0],_imgWorldDims[1]);
        TRACE("Set camera to Image mode");
        break;
    }
    default:
        ERROR("Unkown camera mode: %d", mode);
    }
    _needsRedraw = true;
}

/**
 * \brief Get the current camera mode
 */
Renderer::CameraMode Renderer::getCameraMode() const
{
    return _cameraMode;
}

/**
 * \brief Set the VTK camera parameters based on a 4x4 view matrix
 */
void Renderer::setCameraFromMatrix(vtkCamera *camera, vtkMatrix4x4& mat)
{
    double d = camera->GetDistance();
    double vu[3];
    vu[0] = mat[1][0];
    vu[1] = mat[1][1];
    vu[2] = mat[1][2];
    double trans[3];
    trans[0] = mat[0][3];
    trans[1] = mat[1][3];
    trans[2] = mat[2][3];
    mat[0][3] = 0;
    mat[1][3] = 0;
    mat[2][3] = 0;
    double vpn[3] = {mat[2][0], mat[2][1], mat[2][2]};
    double pos[3];
    // With translation removed, we have an orthogonal matrix, 
    // so the inverse is the transpose
    mat.Transpose();
    mat.MultiplyPoint(trans, pos);
    pos[0] = -pos[0];
    pos[1] = -pos[1];
    pos[2] = -pos[2];
    double fp[3];
    fp[0] = pos[0] - vpn[0] * d;
    fp[1] = pos[1] - vpn[1] * d;
    fp[2] = pos[2] - vpn[2] * d;
    camera->SetPosition(pos);
    camera->SetFocalPoint(fp);
    camera->SetViewUp(vu);
}

/**
 * \brief Set the orientation of the camera from a quaternion
 * rotation
 *
 * \param[in] quat A quaternion with scalar part first: w,x,y,z
 * \param[in] absolute Is rotation absolute or relative?
 */
void Renderer::setCameraOrientation(const double quat[4], bool absolute)
{
    if (_cameraMode == IMAGE)
        return;
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
    vtkSmartPointer<vtkMatrix4x4> rotMat = vtkSmartPointer<vtkMatrix4x4>::New();

    double q[4];
    copyQuat(quat, q);

    if (absolute) {
        double abs[4];
        // Save absolute rotation
        copyQuat(q, abs);
        // Compute relative rotation
        quatMult(quatReciprocal(_cameraOrientation), q, q);
        // Store absolute rotation
        copyQuat(abs, _cameraOrientation);
    } else {
        // Compute new absolute rotation
        quatMult(_cameraOrientation, q, _cameraOrientation);
    }

    quaternionToTransposeMatrix4x4(q, *rotMat);
#ifdef DEBUG
    TRACE("Rotation matrix:\n %g %g %g\n %g %g %g\n %g %g %g",
          (*rotMat)[0][0], (*rotMat)[0][1], (*rotMat)[0][2],
          (*rotMat)[1][0], (*rotMat)[1][1], (*rotMat)[1][2],
          (*rotMat)[2][0], (*rotMat)[2][1], (*rotMat)[2][2]);
    vtkSmartPointer<vtkMatrix4x4> camMat = vtkSmartPointer<vtkMatrix4x4>::New();
    camMat->DeepCopy(camera->GetViewTransformMatrix());
    TRACE("Camera matrix:\n %g %g %g %g\n %g %g %g %g\n %g %g %g %g\n %g %g %g %g",
          (*camMat)[0][0], (*camMat)[0][1], (*camMat)[0][2], (*camMat)[0][3],
          (*camMat)[1][0], (*camMat)[1][1], (*camMat)[1][2], (*camMat)[1][3],
          (*camMat)[2][0], (*camMat)[2][1], (*camMat)[2][2], (*camMat)[2][3],
          (*camMat)[3][0], (*camMat)[3][1], (*camMat)[3][2], (*camMat)[3][3]);
    printCameraInfo(camera);
#endif
    trans->Translate(0, 0, -camera->GetDistance());
    trans->Concatenate(rotMat);
    trans->Translate(0, 0, camera->GetDistance());
    trans->Concatenate(camera->GetViewTransformMatrix());
    setCameraFromMatrix(camera, *trans->GetMatrix());

    _renderer->ResetCameraClippingRange();
    printCameraInfo(camera);
    _needsRedraw = true;
}

/**
 * \brief Set the position and orientation of the camera
 *
 * \param[in] position x,y,z position of camera in world coordinates
 * \param[in] focalPoint x,y,z look-at point in world coordinates
 * \param[in] viewUp x,y,z up vector of camera
 */
void Renderer::setCameraOrientationAndPosition(const double position[3],
                                               const double focalPoint[3],
                                               const double viewUp[3])
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->SetPosition(position);
    camera->SetFocalPoint(focalPoint);
    camera->SetViewUp(viewUp);
    _renderer->ResetCameraClippingRange();
    _needsRedraw = true;
}

/**
 * \brief Get the position and orientation of the camera
 *
 * \param[out] position x,y,z position of camera in world coordinates
 * \param[out] focalPoint x,y,z look-at point in world coordinates
 * \param[out] viewUp x,y,z up vector of camera
 */
void Renderer::getCameraOrientationAndPosition(double position[3],
                                               double focalPoint[3],
                                               double viewUp[3])
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->GetPosition(position);
    camera->GetFocalPoint(focalPoint);
    camera->GetViewUp(viewUp);
}

/**
 * \brief Reset pan, zoom, clipping planes and optionally rotation
 *
 * \param[in] resetOrientation Reset the camera rotation/orientation also
 */
void Renderer::resetCamera(bool resetOrientation)
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    if (_cameraMode == IMAGE) {
        initCamera();
    } else {
        if (resetOrientation) {
            camera->SetPosition(0, 0, 1);
            camera->SetFocalPoint(0, 0, 0);
            camera->SetViewUp(0, 1, 0);
            _cameraOrientation[0] = 1.0;
            _cameraOrientation[1] = 0.0;
            _cameraOrientation[2] = 0.0;
            _cameraOrientation[3] = 0.0;
        }
        //setViewAngle(_windowHeight);
        double bounds[6];
        collectBounds(bounds, false);
        _renderer->ResetCamera(bounds);
        _renderer->ResetCameraClippingRange();
        //computeScreenWorldCoords();
    }

    printCameraInfo(camera);

    _cameraZoomRatio = 1;
    _cameraPan[0] = 0;
    _cameraPan[1] = 0;

    _needsRedraw = true;
}

void Renderer::resetCameraClippingRange()
{
    _renderer->ResetCameraClippingRange();
}

/**
 * \brief Perform a relative rotation to current camera orientation
 *
 * Angles are in degrees, rotation is about focal point
 */
void Renderer::rotateCamera(double yaw, double pitch, double roll)
{
    if (_cameraMode == IMAGE)
        return;
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->Azimuth(yaw); // Rotate about object
    //camera->SetYaw(yaw); // Rotate about camera
    camera->Elevation(pitch); // Rotate about object
    //camera->SetPitch(pitch); // Rotate about camera
    camera->Roll(roll); // Roll about camera view axis
    _renderer->ResetCameraClippingRange();
    //computeScreenWorldCoords();
    _needsRedraw = true;
}

/**
 * \brief Perform a 2D translation of the camera
 *
 * x,y pan amount are specified as signed absolute pan amount in viewport
 * units -- i.e. 0 is no pan, .5 is half the viewport, 2 is twice the viewport,
 * etc.
 *
 * \param[in] x Viewport coordinate horizontal panning (positive number pans
 * camera left, object right)
 * \param[in] y Viewport coordinate vertical panning (positive number pans 
 * camera up, object down)
 * \param[in] absolute Control if pan amount is relative to current or absolute
 */
void Renderer::panCamera(double x, double y, bool absolute)
{
    TRACE("Enter panCamera: %g %g, current abs: %g %g",
          x, y, _cameraPan[0], _cameraPan[1]);

    if (_cameraMode == IMAGE) {
        // Reverse x rather than y, since we are panning the camera, and client
        // expects to be panning/moving the object
        x = -x * _screenWorldCoords[2];
        y = y * _screenWorldCoords[3];

        if (absolute) {
            double panAbs[2];
            panAbs[0] = x;
            panAbs[1] = y;
            x -= _cameraPan[0];
            y -= _cameraPan[1];
            _cameraPan[0] = panAbs[0];
            _cameraPan[1] = panAbs[1];
        } else {
            _cameraPan[0] += x;
            _cameraPan[1] += y;
        }

        _imgWorldOrigin[0] += x;
        _imgWorldOrigin[1] += y;
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0], _imgWorldDims[1]);
    } else {
        y = -y;
        if (absolute) {
            double panAbs[2];
            panAbs[0] = x;
            panAbs[1] = y;
            x -= _cameraPan[0];
            y -= _cameraPan[1];
            _cameraPan[0] = panAbs[0];
            _cameraPan[1] = panAbs[1];
        } else {
            _cameraPan[0] += x;
            _cameraPan[1] += y;
        }

        if (x != 0.0 || y != 0.0) {
            vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
            double viewFocus[4], focalDepth, viewPoint[3];
            double newPickPoint[4], oldPickPoint[4], motionVector[3];

            camera->GetFocalPoint(viewFocus);
            computeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], 
                                  viewFocus);
            focalDepth = viewFocus[2];

            computeDisplayToWorld(( x * 2. + 1.) * _windowWidth / 2.0,
                                  ( y * 2. + 1.) * _windowHeight / 2.0,
                                  focalDepth, 
                                  newPickPoint);

            computeDisplayToWorld(_windowWidth / 2.0,
                                  _windowHeight / 2.0,
                                  focalDepth, 
                                  oldPickPoint);

            // Camera motion is reversed
            motionVector[0] = oldPickPoint[0] - newPickPoint[0];
            motionVector[1] = oldPickPoint[1] - newPickPoint[1];
            motionVector[2] = oldPickPoint[2] - newPickPoint[2];

            camera->GetFocalPoint(viewFocus);
            camera->GetPosition(viewPoint);
            camera->SetFocalPoint(motionVector[0] + viewFocus[0],
                                  motionVector[1] + viewFocus[1],
                                  motionVector[2] + viewFocus[2]);

            camera->SetPosition(motionVector[0] + viewPoint[0],
                                motionVector[1] + viewPoint[1],
                                motionVector[2] + viewPoint[2]);

            _renderer->ResetCameraClippingRange();
            //computeScreenWorldCoords();
        }
    }

    TRACE("Leave panCamera: %g %g, current abs: %g %g",
          x, y, _cameraPan[0], _cameraPan[1]);

    _needsRedraw = true;
}

/**
 * \brief Dolly camera or set orthographic scaling based on camera type
 *
 * \param[in] z Ratio to change zoom (greater than 1 is zoom in, less than 1 is zoom out)
 * \param[in] absolute Control if zoom factor is relative to current setting or absolute
 */
void Renderer::zoomCamera(double z, bool absolute)
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    TRACE("Enter Zoom: current abs: %g, z: %g, view angle %g",
          _cameraZoomRatio, z, camera->GetViewAngle());

    if (absolute) {
        assert(_cameraZoomRatio > 0.0);
        double zAbs = z;
        z *= 1.0/_cameraZoomRatio;
        _cameraZoomRatio = zAbs;
    } else {
        _cameraZoomRatio *= z;
    }

    if (_cameraMode == IMAGE) {
        double dx = _imgWorldDims[0];
        double dy = _imgWorldDims[1];
        _imgWorldDims[0] /= z;
        _imgWorldDims[1] /= z;
        dx -= _imgWorldDims[0];
        dy -= _imgWorldDims[1];
        _imgWorldOrigin[0] += dx/2.0;
        _imgWorldOrigin[1] += dy/2.0;
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0], _imgWorldDims[1]);
    } else {
        // Keep ortho and perspective modes in sync
        // Move camera forward/back for perspective camera
        camera->Dolly(z);
        // Change ortho parallel scale
        camera->SetParallelScale(camera->GetParallelScale()/z);
        _renderer->ResetCameraClippingRange();
        //computeScreenWorldCoords();
    }

    TRACE("Leave Zoom: rel %g, new abs: %g, view angle %g", 
          z, _cameraZoomRatio, camera->GetViewAngle());

    _needsRedraw = true;
}

/**
 * \brief Set the pan/zoom using a corner and dimensions in pixel coordinates
 * 
 * \param[in] x left pixel coordinate
 * \param[in] y bottom  pixel coordinate (with y=0 at top of window)
 * \param[in] width Width of zoom region in pixel coordinates
 * \param[in] height Height of zoom region in pixel coordinates
 */
void Renderer::setCameraZoomRegionPixels(int x, int y, int width, int height)
{
    double wx, wy, ww, wh;

    y = _windowHeight - y;
    double pxToWorldX = _screenWorldCoords[2] / (double)_windowWidth;
    double pxToWorldY = _screenWorldCoords[3] / (double)_windowHeight;

    wx = _screenWorldCoords[0] + x * pxToWorldX;
    wy = _screenWorldCoords[1] + y * pxToWorldY;
    ww = abs(width) *  pxToWorldX;
    wh = abs(height) * pxToWorldY;
    setCameraZoomRegion(wx, wy, ww, wh);

    TRACE("\npx: %d %d %d %d\nworld: %g %g %g %g",
          x, y, width, height,
          wx, wy, ww, wh);
}

/**
 * \brief Set the pan/zoom using a corner and dimensions in world coordinates
 * 
 * \param[in] x left world coordinate
 * \param[in] y bottom  world coordinate
 * \param[in] width Width of zoom region in world coordinates
 * \param[in] height Height of zoom region in world coordinates
 */
void Renderer::setCameraZoomRegion(double x, double y, double width, double height)
{
    double camPos[2];

    int pxOffsetX = (int)(0.17 * (double)_windowWidth);
    pxOffsetX = (pxOffsetX > 100 ? 100 : pxOffsetX);
    int pxOffsetY = (int)(0.15 * (double)_windowHeight);
    pxOffsetY = (pxOffsetY > 75 ? 75 : pxOffsetY);
    int outerGutter = (int)(0.03 * (double)_windowWidth);
    outerGutter = (outerGutter > 15 ? 15 : outerGutter);

    int imgHeightPx = _windowHeight - pxOffsetY - outerGutter;
    int imgWidthPx = _windowWidth - pxOffsetX - outerGutter;

    double imgAspect = width / height;
    double winAspect = (double)_windowWidth / _windowHeight;

    double pxToWorld;

    if (imgAspect >= winAspect) {
        pxToWorld = width / imgWidthPx;
    } else {
        pxToWorld = height / imgHeightPx;
    }

    double offsetX = pxOffsetX * pxToWorld;
    double offsetY = pxOffsetY * pxToWorld;

    TRACE("Window: %d %d", _windowWidth, _windowHeight);
    TRACE("ZoomRegion: %g %g %g %g", x, y, width, height);
    TRACE("pxToWorld: %g", pxToWorld);
    TRACE("offset: %g %g", offsetX, offsetY);

    setCameraMode(IMAGE);

    _imgWorldOrigin[0] = x;
    _imgWorldOrigin[1] = y;
    _imgWorldDims[0] = width;
    _imgWorldDims[1] = height;

    camPos[0] = _imgWorldOrigin[0] - offsetX + (_windowWidth * pxToWorld)/2.0;
    camPos[1] = _imgWorldOrigin[1] - offsetY + (_windowHeight * pxToWorld)/2.0;

    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->ParallelProjectionOn();
    camera->SetClippingRange(1, 2);
    // Half of world coordinate height of viewport (Documentation is wrong)
    camera->SetParallelScale(_windowHeight * pxToWorld / 2.0);

    if (_imgCameraPlane == PLANE_XY) {
        // XY plane
        camera->SetPosition(camPos[0], camPos[1], _imgCameraOffset + 1.);
        camera->SetFocalPoint(camPos[0], camPos[1], _imgCameraOffset);
        camera->SetViewUp(0, 1, 0);
        // bottom
        _cameraClipPlanes[0]->SetOrigin(0, _imgWorldOrigin[1], 0);
        _cameraClipPlanes[0]->SetNormal(0, 1, 0);
        // left
        _cameraClipPlanes[1]->SetOrigin(_imgWorldOrigin[0], 0, 0);
        _cameraClipPlanes[1]->SetNormal(1, 0, 0);
        // top
        _cameraClipPlanes[2]->SetOrigin(0, _imgWorldOrigin[1] + _imgWorldDims[1], 0);
        _cameraClipPlanes[2]->SetNormal(0, -1, 0);
        // right
        _cameraClipPlanes[3]->SetOrigin(_imgWorldOrigin[0] + _imgWorldDims[0], 0, 0);
        _cameraClipPlanes[3]->SetNormal(-1, 0, 0);
        _cubeAxesActor2D->SetBounds(_imgWorldOrigin[0], _imgWorldOrigin[0] + _imgWorldDims[0],
                                    _imgWorldOrigin[1], _imgWorldOrigin[1] + _imgWorldDims[1],
                                    _imgCameraOffset, _imgCameraOffset);
        _cubeAxesActor2D->XAxisVisibilityOn();
        _cubeAxesActor2D->YAxisVisibilityOn();
        _cubeAxesActor2D->ZAxisVisibilityOff();
    } else if (_imgCameraPlane == PLANE_ZY) {
        // ZY plane
        camera->SetPosition(_imgCameraOffset - 1., camPos[1], camPos[0]);
        camera->SetFocalPoint(_imgCameraOffset, camPos[1], camPos[0]);
        camera->SetViewUp(0, 1, 0);
        // bottom
        _cameraClipPlanes[0]->SetOrigin(0, _imgWorldOrigin[1], 0);
        _cameraClipPlanes[0]->SetNormal(0, 1, 0);
        // left
        _cameraClipPlanes[1]->SetOrigin(0, 0, _imgWorldOrigin[0]);
        _cameraClipPlanes[1]->SetNormal(0, 0, 1);
        // top
        _cameraClipPlanes[2]->SetOrigin(0, _imgWorldOrigin[1] + _imgWorldDims[1], 0);
        _cameraClipPlanes[2]->SetNormal(0, -1, 0);
        // right
        _cameraClipPlanes[3]->SetOrigin(0, 0, _imgWorldOrigin[0] + _imgWorldDims[0]);
        _cameraClipPlanes[3]->SetNormal(0, 0, -1);
        _cubeAxesActor2D->SetBounds(_imgCameraOffset, _imgCameraOffset, 
                                    _imgWorldOrigin[1], _imgWorldOrigin[1] + _imgWorldDims[1],
                                    _imgWorldOrigin[0], _imgWorldOrigin[0] + _imgWorldDims[0]);
        _cubeAxesActor2D->XAxisVisibilityOff();
        _cubeAxesActor2D->YAxisVisibilityOn();
        _cubeAxesActor2D->ZAxisVisibilityOn();
    } else {
        // XZ plane
        camera->SetPosition(camPos[0], _imgCameraOffset - 1., camPos[1]);
        camera->SetFocalPoint(camPos[0], _imgCameraOffset, camPos[1]);
        camera->SetViewUp(0, 0, 1);
        // bottom
        _cameraClipPlanes[0]->SetOrigin(0, 0, _imgWorldOrigin[1]);
        _cameraClipPlanes[0]->SetNormal(0, 0, 1);
        // left
        _cameraClipPlanes[1]->SetOrigin(_imgWorldOrigin[0], 0, 0);
        _cameraClipPlanes[1]->SetNormal(1, 0, 0);
        // top
        _cameraClipPlanes[2]->SetOrigin(0, 0, _imgWorldOrigin[1] + _imgWorldDims[1]);
        _cameraClipPlanes[2]->SetNormal(0, 0, -1);
        // right
        _cameraClipPlanes[3]->SetOrigin(_imgWorldOrigin[0] + _imgWorldDims[0], 0, 0);
        _cameraClipPlanes[3]->SetNormal(-1, 0, 0);
        _cubeAxesActor2D->SetBounds(_imgWorldOrigin[0], _imgWorldOrigin[0] + _imgWorldDims[0],
                                    _imgCameraOffset, _imgCameraOffset,
                                    _imgWorldOrigin[1], _imgWorldOrigin[1] + _imgWorldDims[1]);
        _cubeAxesActor2D->XAxisVisibilityOn();
        _cubeAxesActor2D->YAxisVisibilityOff();
        _cubeAxesActor2D->ZAxisVisibilityOn();
    }

    // Compute screen world coordinates
    computeScreenWorldCoords();

#ifdef DEBUG
    printCameraInfo(camera);
#endif

    _needsRedraw = true;
}

/**
 * \brief Convert pixel/display coordinates to world coordinates based on current camera
 */
void Renderer::computeDisplayToWorld(double x, double y, double z, double worldPt[4])
{
    _renderer->SetDisplayPoint(x, y, z);
    _renderer->DisplayToWorld();
    _renderer->GetWorldPoint(worldPt);
    if (worldPt[3]) {
        worldPt[0] /= worldPt[3];
        worldPt[1] /= worldPt[3];
        worldPt[2] /= worldPt[3];
        worldPt[3] = 1.0;
    }
}

/**
 * \brief Convert world coordinates to pixel/display coordinates based on current camera
 */
void Renderer::computeWorldToDisplay(double x, double y, double z, double displayPt[3])
{
    _renderer->SetWorldPoint(x, y, z, 1.0);
    _renderer->WorldToDisplay();
    _renderer->GetDisplayPoint(displayPt);
}

/**
 * \brief Compute the world coordinate bounds of the display rectangle
 */
void Renderer::computeScreenWorldCoords()
{
    // Start with viewport coords [-1,1]
    double x0 = -1;
    double y0 = -1;
    double z0 = -1;
    double x1 = 1;
    double y1 = 1;
    double z1 = -1;

    vtkMatrix4x4 *mat = vtkMatrix4x4::New();
    double result[4];

    // get the perspective transformation from the active camera
    mat->DeepCopy(_renderer->GetActiveCamera()->
                  GetCompositeProjectionTransformMatrix(_renderer->GetTiledAspectRatio(),0,1));

    // use the inverse matrix
    mat->Invert();

    // Transform point to world coordinates
    result[0] = x0;
    result[1] = y0;
    result[2] = z0;
    result[3] = 1.0;

    mat->MultiplyPoint(result, result);

    // Get the transformed vector & set WorldPoint
    // while we are at it try to keep w at one
    if (result[3]) {
        x0 = result[0] / result[3];
        y0 = result[1] / result[3];
        z0 = result[2] / result[3];
    }

    result[0] = x1;
    result[1] = y1;
    result[2] = z1;
    result[3] = 1.0;

    mat->MultiplyPoint(result, result);

    if (result[3]) {
        x1 = result[0] / result[3];
        y1 = result[1] / result[3];
        z1 = result[2] / result[3];
    }

    mat->Delete();

    if (_imgCameraPlane == PLANE_XZ) {
        _screenWorldCoords[0] = x0;
        _screenWorldCoords[1] = z0;
        _screenWorldCoords[2] = x1 - x0;
        _screenWorldCoords[3] = z1 - z0;
    } else if (_imgCameraPlane == PLANE_ZY) {
        _screenWorldCoords[0] = z0;
        _screenWorldCoords[1] = y0;
        _screenWorldCoords[2] = z1 - z0;
        _screenWorldCoords[3] = y1 - y0;
    } else {
        // XY
        _screenWorldCoords[0] = x0;
        _screenWorldCoords[1] = y0;
        _screenWorldCoords[2] = x1 - x0;
        _screenWorldCoords[3] = y1 - y0;
    }
}

/**
 * \brief Get the world coordinates of the image camera plot area
 *
 * \param[out] xywh Array to hold x,y,width,height world coordinates
 */
void Renderer::getCameraZoomRegion(double xywh[4]) const
{
    xywh[0] = _imgWorldOrigin[0];
    xywh[1] = _imgWorldOrigin[1];
    xywh[2] = _imgWorldDims[0];
    xywh[3] = _imgWorldDims[1];
}

/**
 * \brief Get the world origin and dimensions of the screen
 *
 * \param[out] xywh Array to hold x,y,width,height world coordinates
 */
void Renderer::getScreenWorldCoords(double xywh[4]) const
{
    memcpy(xywh, _screenWorldCoords, sizeof(double)*4);
}

/**
 * \brief Compute bounding box containing the two input bounding boxes
 *
 * \param[out] boundsDest Union of the two input bounding boxes
 * \param[in] bounds1 Input bounding box
 * \param[in] bounds2 Input bounding box
 */
void Renderer::mergeBounds(double *boundsDest,
                           const double *bounds1, const double *bounds2)
{
    assert(boundsDest != NULL);
    assert(bounds1 != NULL);
    if (bounds2 == NULL) {
        WARN("NULL bounds2 array");
        return;
    }
    for (int i = 0; i < 6; i++) {
        if (i % 2 == 0)
            boundsDest[i] = min2(bounds1[i], bounds2[i]);
        else
            boundsDest[i] = max2(bounds1[i], bounds2[i]);
    }
}

/**
 * \brief Collect bounds of all graphics objects
 *
 * \param[out] bounds Bounds of all scene objects
 * \param[in] onlyVisible Only collect bounds of visible objects
 */
void Renderer::collectBounds(double *bounds, bool onlyVisible)
{
    bounds[0] = DBL_MAX;
    bounds[1] = -DBL_MAX;
    bounds[2] = DBL_MAX;
    bounds[3] = -DBL_MAX;
    bounds[4] = DBL_MAX;
    bounds[5] = -DBL_MAX;

    for (DataSetHashmap::iterator itr = _dataSets.begin();
             itr != _dataSets.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (Contour2DHashmap::iterator itr = _contour2Ds.begin();
             itr != _contour2Ds.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (Contour3DHashmap::iterator itr = _contour3Ds.begin();
             itr != _contour3Ds.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (CutplaneHashmap::iterator itr = _cutplanes.begin();
             itr != _cutplanes.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (GlyphsHashmap::iterator itr = _glyphs.begin();
             itr != _glyphs.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (HeightMapHashmap::iterator itr = _heightMaps.begin();
             itr != _heightMaps.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (LICHashmap::iterator itr = _lics.begin();
             itr != _lics.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (MoleculeHashmap::iterator itr = _molecules.begin();
             itr != _molecules.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (PolyDataHashmap::iterator itr = _polyDatas.begin();
             itr != _polyDatas.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
             itr != _pseudoColors.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (StreamlinesHashmap::iterator itr = _streamlines.begin();
             itr != _streamlines.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }
    for (VolumeHashmap::iterator itr = _volumes.begin();
             itr != _volumes.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getProp()->GetBounds());
    }

    for (int i = 0; i < 6; i += 2) {
        if (bounds[i+1] < bounds[i]) {
            bounds[i] = -0.5;
            bounds[i+1] = 0.5;
        }
    }

    int numDims = 0;
    if (bounds[0] != bounds[1])
        numDims++;
    if (bounds[2] != bounds[3])
        numDims++;
    if (bounds[4] != bounds[5])
        numDims++;

    if (numDims == 0) {
        bounds[0] -= .5;
        bounds[1] += .5;
        bounds[2] -= .5;
        bounds[3] += .5;
    }

    TRACE("Bounds: %g %g %g %g %g %g",
          bounds[0],
          bounds[1],
          bounds[2],
          bounds[3],
          bounds[4],
          bounds[5]);
}

/**
 * \brief Update data ranges for color-mapping and contours
 */
void Renderer::updateFieldRanges()
{
    collectDataRanges();

    for (Contour2DHashmap::iterator itr = _contour2Ds.begin();
         itr != _contour2Ds.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (Contour3DHashmap::iterator itr = _contour3Ds.begin();
         itr != _contour3Ds.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (CutplaneHashmap::iterator itr = _cutplanes.begin();
         itr != _cutplanes.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (GlyphsHashmap::iterator itr = _glyphs.begin();
         itr != _glyphs.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (HeightMapHashmap::iterator itr = _heightMaps.begin();
         itr != _heightMaps.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (LICHashmap::iterator itr = _lics.begin();
         itr != _lics.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (MoleculeHashmap::iterator itr = _molecules.begin();
         itr != _molecules.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
         itr != _pseudoColors.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (StreamlinesHashmap::iterator itr = _streamlines.begin();
         itr != _streamlines.end(); ++itr) {
        itr->second->updateRanges(this);
    }
    for (VolumeHashmap::iterator itr = _volumes.begin();
         itr != _volumes.end(); ++itr) {
        itr->second->updateRanges(this);
    }
}

/**
 * \brief Collect cumulative data range of all DataSets
 *
 * \param[out] range Data range of all DataSets
 * \param[in] name Field name
 * \param[in] type Attribute type: e.g. POINT_DATA, CELL_DATA
 * \param[in] component Array component or -1 for magnitude
 * \param[in] onlyVisible Only collect range of visible DataSets
 */
void Renderer::collectDataRanges(double *range, const char *name,
                                 DataSet::DataAttributeType type,
                                 int component, bool onlyVisible)
{
    range[0] = DBL_MAX;
    range[1] = -DBL_MAX;

    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility()) {
            double r[2];
            itr->second->getDataRange(r, name, type, component);
            range[0] = min2(range[0], r[0]);
            range[1] = max2(range[1], r[1]);
        }
    }
    if (range[0] == DBL_MAX)
        range[0] = 0;
    if (range[1] == -DBL_MAX)
        range[1] = 1;
    
    TRACE("n:'%s' t:%d c:%d [%g,%g]", name, type, component,
          range[0], range[1]);
}

/**
 * \brief Clear field range hashtables and free memory
 */
void Renderer::clearFieldRanges()
{
    TRACE("Deleting Field Ranges");
    for (FieldRangeHashmap::iterator itr = _scalarPointDataRange.begin();
         itr != _scalarPointDataRange.end(); ++itr) {
        delete [] itr->second;
    }
    _scalarPointDataRange.clear();
    for (FieldRangeHashmap::iterator itr = _scalarCellDataRange.begin();
         itr != _scalarCellDataRange.end(); ++itr) {
        delete [] itr->second;
    }
    _scalarCellDataRange.clear();
    for (FieldRangeHashmap::iterator itr = _vectorPointDataRange.begin();
         itr != _vectorPointDataRange.end(); ++itr) {
        delete [] itr->second;
    }
    _vectorPointDataRange.clear();
    for (int i = 0; i < 3; i++) {
        for (FieldRangeHashmap::iterator itr = _vectorCompPointDataRange[i].begin();
             itr != _vectorCompPointDataRange[i].end(); ++itr) {
            delete [] itr->second;
        }
        _vectorCompPointDataRange[i].clear();
    }
    for (FieldRangeHashmap::iterator itr = _vectorCellDataRange.begin();
         itr != _vectorCellDataRange.end(); ++itr) {
        delete [] itr->second;
    }
    _vectorCellDataRange.clear();
    for (int i = 0; i < 3; i++) {
        for (FieldRangeHashmap::iterator itr = _vectorCompCellDataRange[i].begin();
             itr != _vectorCompCellDataRange[i].end(); ++itr) {
            delete [] itr->second;
        }
        _vectorCompCellDataRange[i].clear();
    }
}

void Renderer::initFieldRanges()
{
    clearFieldRanges();

    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        DataSet *ds = itr->second;
        std::vector<std::string> names;
        ds->getFieldNames(names, DataSet::POINT_DATA, 1);
        for (std::vector<std::string>::iterator itr = names.begin();
             itr != names.end(); ++itr) {
            FieldRangeHashmap::iterator fritr = 
                _scalarPointDataRange.find(*itr);
            if (fritr == _scalarPointDataRange.end()) {
                _scalarPointDataRange[*itr] = new double[2];
                _scalarPointDataRange[*itr][0] = 0;
                _scalarPointDataRange[*itr][1] = 1;
            }
        }
        names.clear();
        ds->getFieldNames(names, DataSet::CELL_DATA, 1);
        for (std::vector<std::string>::iterator itr = names.begin();
             itr != names.end(); ++itr) {
            FieldRangeHashmap::iterator fritr = 
                _scalarCellDataRange.find(*itr);
            if (fritr == _scalarCellDataRange.end()) {
                _scalarCellDataRange[*itr] = new double[2];
                _scalarCellDataRange[*itr][0] = 0;
                _scalarCellDataRange[*itr][1] = 1;
            }
        }
        names.clear();
        ds->getFieldNames(names, DataSet::POINT_DATA, 3);
        for (std::vector<std::string>::iterator itr = names.begin();
             itr != names.end(); ++itr) {
            FieldRangeHashmap::iterator fritr = 
                _vectorPointDataRange.find(*itr);
            if (fritr == _vectorPointDataRange.end()) {
                _vectorPointDataRange[*itr] = new double[2];
                _vectorPointDataRange[*itr][0] = 0;
                _vectorPointDataRange[*itr][1] = 1;
            }
            for (int i = 0; i < 3; i++) {
                fritr = _vectorCompPointDataRange[i].find(*itr);
                if (fritr == _vectorCompPointDataRange[i].end()) {
                    _vectorCompPointDataRange[i][*itr] = new double[2];
                    _vectorCompPointDataRange[i][*itr][0] = 0;
                    _vectorCompPointDataRange[i][*itr][1] = 1;
                }
            }
        }
        names.clear();
        ds->getFieldNames(names, DataSet::CELL_DATA, 3);
        for (std::vector<std::string>::iterator itr = names.begin();
             itr != names.end(); ++itr) {
            FieldRangeHashmap::iterator fritr = 
                _vectorCellDataRange.find(*itr);
            if (fritr == _vectorCellDataRange.end()) {
                _vectorCellDataRange[*itr] = new double[2];
                _vectorCellDataRange[*itr][0] = 0;
                _vectorCellDataRange[*itr][1] = 1;
            }
            for (int i = 0; i < 3; i++) {
                fritr = _vectorCompCellDataRange[i].find(*itr);
                if (fritr == _vectorCompCellDataRange[i].end()) {
                    _vectorCompCellDataRange[i][*itr] = new double[2];
                    _vectorCompCellDataRange[i][*itr][0] = 0;
                    _vectorCompCellDataRange[i][*itr][1] = 1;
                }
            }
        }
    }
}

/**
 * \brief Returns boolean flag indicating if cumulative data ranges 
 * should be used
 */
bool Renderer::getUseCumulativeRange()
{
    return _useCumulativeRange;
}

/**
 * \brief Get the cumulative range across all DataSets for a point
 * data field if it exists, otherwise a cell data field if it exists
 *
 * \param[out] range Pointer to an array of 2 doubles
 * \param[in] name Field name
 * \param[in] numComponents Number of components in field
 * \param[in] component Index of component or -1 for magnitude/scalar
 * \return boolean indicating if field was found
 */
bool Renderer::getCumulativeDataRange(double *range, const char *name,
                                      int numComponents,
                                      int component)
{
    bool ret;
    if (!(ret = getCumulativeDataRange(range, name, DataSet::POINT_DATA,
                                       numComponents, component)))
        ret = getCumulativeDataRange(range, name, DataSet::CELL_DATA,
                                     numComponents, component);
    return ret;
}

/**
 * \brief Get the cumulative range across all DataSets for a field
 *
 * \param[out] range Pointer to an array of 2 doubles
 * \param[in] name Field name
 * \param[in] type DataAttributeType of field
 * \param[in] numComponents Number of components in field
 * \param[in] component Index of component or -1 for magnitude/scalar
 * \return boolean indicating if field was found
 */
bool Renderer::getCumulativeDataRange(double *range, const char *name,
                                      DataSet::DataAttributeType type,
                                      int numComponents,
                                      int component)
{
    if (name == NULL)
        return false;
    switch (type) {
    case DataSet::POINT_DATA: {
        FieldRangeHashmap::iterator itr;
        if (numComponents == 1) {
            itr = _scalarPointDataRange.find(name);
            if (itr == _scalarPointDataRange.end()) {
                return false;
            } else {
                memcpy(range, itr->second, sizeof(double)*2);
                return true;
            }
        } else if (numComponents == 3) {
            if (component == -1) {
                itr = _vectorPointDataRange.find(name);
                if (itr == _vectorPointDataRange.end()) {
                    return false;
                } else {
                    memcpy(range, itr->second, sizeof(double)*2);
                    return true;
                }
            } else if (component >= 0 && component <= 3) {
                itr = _vectorCompPointDataRange[component].find(name);
                if (itr == _vectorCompPointDataRange[component].end()) {
                    return false;
                } else {
                    memcpy(range, itr->second, sizeof(double)*2);
                    return true;
                }
            }
        }
    }
        break;
    case DataSet::CELL_DATA: {
        FieldRangeHashmap::iterator itr;
        if (numComponents == 1) {
            itr = _scalarCellDataRange.find(name);
            if (itr == _scalarCellDataRange.end()) {
                return false;
            } else {
                memcpy(range, itr->second, sizeof(double)*2);
                return true;
            }
        } else if (numComponents == 3) {
            if (component == -1) {
                itr = _vectorCellDataRange.find(name);
                if (itr == _vectorCellDataRange.end()) {
                    return false;
                } else {
                    memcpy(range, itr->second, sizeof(double)*2);
                    return true;
                }
            } else if (component >= 0 && component <= 3) {
                itr = _vectorCompCellDataRange[component].find(name);
                if (itr == _vectorCompCellDataRange[component].end()) {
                    return false;
                } else {
                    memcpy(range, itr->second, sizeof(double)*2);
                    return true;
                }
            }
        }
    }
        break;
    default:
        break;
    }
    return false;
}

void Renderer::collectDataRanges()
{
    for (FieldRangeHashmap::iterator itr = _scalarPointDataRange.begin();
         itr != _scalarPointDataRange.end(); ++itr) {
        collectDataRanges(itr->second, itr->first.c_str(),
                          DataSet::POINT_DATA, -1,
                          _cumulativeRangeOnlyVisible);
    }
    for (FieldRangeHashmap::iterator itr = _scalarCellDataRange.begin();
         itr != _scalarCellDataRange.end(); ++itr) {
        collectDataRanges(itr->second, itr->first.c_str(),
                          DataSet::CELL_DATA, -1,
                          _cumulativeRangeOnlyVisible);
    }
    for (FieldRangeHashmap::iterator itr = _vectorPointDataRange.begin();
         itr != _vectorPointDataRange.end(); ++itr) {
        collectDataRanges(itr->second, itr->first.c_str(),
                          DataSet::POINT_DATA, -1,
                          _cumulativeRangeOnlyVisible);
    }
    for (int i = 0; i < 3; i++) {
        for (FieldRangeHashmap::iterator itr = _vectorCompPointDataRange[i].begin();
             itr != _vectorCompPointDataRange[i].end(); ++itr) {
            collectDataRanges(itr->second, itr->first.c_str(),
                              DataSet::POINT_DATA, i,
                              _cumulativeRangeOnlyVisible);
        }
    }
    for (FieldRangeHashmap::iterator itr = _vectorCellDataRange.begin();
         itr != _vectorCellDataRange.end(); ++itr) {
        collectDataRanges(itr->second, itr->first.c_str(),
                          DataSet::CELL_DATA, -1,
                          _cumulativeRangeOnlyVisible);
    }
    for (int i = 0; i < 3; i++) {
        for (FieldRangeHashmap::iterator itr = _vectorCompCellDataRange[i].begin();
             itr != _vectorCompCellDataRange[i].end(); ++itr) {
            collectDataRanges(itr->second, itr->first.c_str(),
                              DataSet::CELL_DATA, i,
                              _cumulativeRangeOnlyVisible);
        }
    }
}

/**
 * \brief Determines if AABB lies in a principal axis plane
 * and if so, returns the plane normal
 */
bool Renderer::is2D(const double bounds[6],
                    PrincipalPlane *plane,
                    double *offset) const
{
    if (bounds[4] == bounds[5]) {
        // Z = 0, XY plane
        if (plane)
            *plane = PLANE_XY;
        if (offset)
            *offset = bounds[4];
        return true;
    } else if (bounds[0] == bounds[1]) {
        // X = 0, ZY plane
        if (plane)
            *plane = PLANE_ZY;
        if (offset)
            *offset = bounds[0];
        return true;
    } else if (bounds[2] == bounds[3]) {
        // Y = 0, XZ plane
        if (plane)
            *plane = PLANE_XZ;
        if (offset)
            *offset = bounds[2];
        return true;
    }
    *plane = PLANE_XY;
    *offset = 0;
    return false;
}

/**
 * \brief Initialize the camera zoom region to include the bounding volume given
 */
void Renderer::initCamera()
{
#ifdef WANT_TRACE
    switch (_cameraMode) {
    case IMAGE:
        TRACE("Image camera");
        break;
    case ORTHO:
        TRACE("Ortho camera");
        break;
    case PERSPECTIVE:
        TRACE("Perspective camera");
        break;
    default:
        TRACE("Unknown camera mode");
    }
#endif
    double bounds[6];
    collectBounds(bounds, false);
    bool twod = is2D(bounds, &_imgCameraPlane, &_imgCameraOffset);
    if (twod) {
        _cameraMode = IMAGE;
        if (_imgCameraPlane == PLANE_ZY) {
            _imgWorldOrigin[0] = bounds[4];
            _imgWorldOrigin[1] = bounds[2];
            _imgWorldDims[0] = bounds[5] - bounds[4];
            _imgWorldDims[1] = bounds[3] - bounds[2];
        } else if (_imgCameraPlane == PLANE_XZ) {
            _imgWorldOrigin[0] = bounds[0];
            _imgWorldOrigin[1] = bounds[4];
            _imgWorldDims[0] = bounds[1] - bounds[0];
            _imgWorldDims[1] = bounds[5] - bounds[4];
        } else {
            _imgWorldOrigin[0] = bounds[0];
            _imgWorldOrigin[1] = bounds[2];
            _imgWorldDims[0] = bounds[1] - bounds[0];
            _imgWorldDims[1] = bounds[3] - bounds[2];
        }
    } else {
        _imgWorldOrigin[0] = bounds[0];
        _imgWorldOrigin[1] = bounds[2];
        _imgWorldDims[0] = bounds[1] - bounds[0];
        _imgWorldDims[1] = bounds[3] - bounds[2];
    }

    _cameraPan[0] = 0;
    _cameraPan[1] = 0;
    _cameraZoomRatio = 1;

    switch (_cameraMode) {
    case IMAGE:
        _renderer->ResetCamera(bounds);
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0], _imgWorldDims[1]);
        resetAxes();
        break;
    case ORTHO:
        _renderer->GetActiveCamera()->ParallelProjectionOn();
        resetAxes(bounds);
        _renderer->ResetCamera(bounds);
        //computeScreenWorldCoords();
        break;
    case PERSPECTIVE:
        _renderer->GetActiveCamera()->ParallelProjectionOff();
        resetAxes(bounds);
        _renderer->ResetCamera(bounds);
        //computeScreenWorldCoords();
        break;
    default:
        ERROR("Unknown camera mode");
    }

#ifdef WANT_TRACE
    printCameraInfo(_renderer->GetActiveCamera());
#endif
}

/**
 * \brief Print debugging info about a vtkCamera
 */
void Renderer::printCameraInfo(vtkCamera *camera)
{
    TRACE("pscale: %g, angle: %g, d: %g pos: %g %g %g, fpt: %g %g %g, vup: %g %g %g, clip: %g %g",
          camera->GetParallelScale(),
          camera->GetViewAngle(),
          camera->GetDistance(),
          camera->GetPosition()[0],
          camera->GetPosition()[1],
          camera->GetPosition()[2], 
          camera->GetFocalPoint()[0],
          camera->GetFocalPoint()[1],
          camera->GetFocalPoint()[2],
          camera->GetViewUp()[0],
          camera->GetViewUp()[1],
          camera->GetViewUp()[2],
          camera->GetClippingRange()[0],
          camera->GetClippingRange()[1]);
}

/**
 * \brief Set the RGB background color to render into the image
 */
void Renderer::setBackgroundColor(float color[3])
{
    _bgColor[0] = color[0];
    _bgColor[1] = color[1];
    _bgColor[2] = color[2];
    _renderer->SetBackground(_bgColor[0], _bgColor[1], _bgColor[2]);
    _needsRedraw = true;
}

/**
 * \brief Set the opacity of the specified DataSet's associated graphics objects
 */
void Renderer::setDataSetOpacity(const DataSetId& id, double opacity)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
        doAll = true;
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _dataSets.end());

    if (id.compare("all") == 0 || getGraphicsObject<Contour2D>(id) != NULL)
        setGraphicsObjectOpacity<Contour2D>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<Contour3D>(id) != NULL)
        setGraphicsObjectOpacity<Contour3D>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<Cutplane>(id) != NULL)
        setGraphicsObjectOpacity<Cutplane>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<Glyphs>(id) != NULL)
        setGraphicsObjectOpacity<Glyphs>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<HeightMap>(id) != NULL)
        setGraphicsObjectOpacity<HeightMap>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<LIC>(id) != NULL)
        setGraphicsObjectOpacity<LIC>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<Molecule>(id) != NULL)
        setGraphicsObjectOpacity<Molecule>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<PolyData>(id) != NULL)
        setGraphicsObjectOpacity<PolyData>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<PseudoColor>(id) != NULL)
        setGraphicsObjectOpacity<PseudoColor>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<Streamlines>(id) != NULL)
        setGraphicsObjectOpacity<Streamlines>(id, opacity);
    if (id.compare("all") == 0 || getGraphicsObject<Volume>(id) != NULL)
        setGraphicsObjectOpacity<Volume>(id, opacity);

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the specified DataSet's associated graphics objects
 */
void Renderer::setDataSetVisibility(const DataSetId& id, bool state)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
        doAll = true;
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _dataSets.end());

    if (id.compare("all") == 0 || getGraphicsObject<Contour2D>(id) != NULL)
        setGraphicsObjectVisibility<Contour2D>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<Contour3D>(id) != NULL)
        setGraphicsObjectVisibility<Contour3D>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<Cutplane>(id) != NULL)
        setGraphicsObjectVisibility<Cutplane>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<Glyphs>(id) != NULL)
        setGraphicsObjectVisibility<Glyphs>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<HeightMap>(id) != NULL)
        setGraphicsObjectVisibility<HeightMap>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<LIC>(id) != NULL)
        setGraphicsObjectVisibility<LIC>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<Molecule>(id) != NULL)
        setGraphicsObjectVisibility<Molecule>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<PolyData>(id) != NULL)
        setGraphicsObjectVisibility<PolyData>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<PseudoColor>(id) != NULL)
        setGraphicsObjectVisibility<PseudoColor>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<Streamlines>(id) != NULL)
        setGraphicsObjectVisibility<Streamlines>(id, state);
    if (id.compare("all") == 0 || getGraphicsObject<Volume>(id) != NULL)
        setGraphicsObjectVisibility<Volume>(id, state);

    _needsRedraw = true;
}

/**
 * \brief Toggle rendering of actors' bounding box
 */
void Renderer::setDataSetShowBounds(const DataSetId& id, bool state)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
        doAll = true;
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return;
    }

    do {
        if (!state && itr->second->getProp()) {
            _renderer->RemoveViewProp(itr->second->getProp());
        }

        itr->second->showOutline(state);

        if (state && !_renderer->HasViewProp(itr->second->getProp())) {
            _renderer->AddViewProp(itr->second->getProp());
        }
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Set color of outline bounding box
 */
void Renderer::setDataSetOutlineColor(const DataSetId& id, float color[3])
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
        doAll = true;
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return;
    }

    do {
        itr->second->setOutlineColor(color);
    } while (doAll && ++itr != _dataSets.end());

    _needsRedraw = true;
}

/**
 * \brief Set a user clipping plane
 *
 * TODO: Fix clip plane positions after a change in actor bounds
 */
void Renderer::setClipPlane(Axis axis, double ratio, int direction)
{
    double bounds[6];
    collectBounds(bounds, false);

    switch (axis) {
    case X_AXIS:
        if (direction > 0) {
            if (ratio > 0.0) {
                if (_userClipPlanes[0] == NULL) {
                    _userClipPlanes[0] = vtkSmartPointer<vtkPlane>::New();
                    _userClipPlanes[0]->SetNormal(1, 0, 0);
                }
                _userClipPlanes[0]->SetOrigin(bounds[0] + (bounds[1]-bounds[0])*ratio, 0, 0);
            } else {
                _userClipPlanes[0] = NULL;
            }
        } else {
            if (ratio < 1.0) {
                if (_userClipPlanes[1] == NULL) {
                    _userClipPlanes[1] = vtkSmartPointer<vtkPlane>::New();
                    _userClipPlanes[1]->SetNormal(-1, 0, 0);
                }
                _userClipPlanes[1]->SetOrigin(bounds[0] + (bounds[1]-bounds[0])*ratio, 0, 0);
            } else {
                _userClipPlanes[1] = NULL;
            }
        }
        break;
    case Y_AXIS:
        if (direction > 0) {
            if (ratio > 0.0) {
                if (_userClipPlanes[2] == NULL) {
                    _userClipPlanes[2] = vtkSmartPointer<vtkPlane>::New();
                    _userClipPlanes[2]->SetNormal(0, 1, 0);
                }
                _userClipPlanes[2]->SetOrigin(0, bounds[2] + (bounds[3]-bounds[2])*ratio, 0);
            } else {
                _userClipPlanes[2] = NULL;
            }
        } else {
            if (ratio < 1.0) {
                if (_userClipPlanes[3] == NULL) {
                    _userClipPlanes[3] = vtkSmartPointer<vtkPlane>::New();
                    _userClipPlanes[3]->SetNormal(0, -1, 0);
                }
                _userClipPlanes[3]->SetOrigin(0, bounds[2] + (bounds[3]-bounds[2])*ratio, 0);
            } else {
                _userClipPlanes[3] = NULL;
            }
        }
        break;
    case Z_AXIS:
        if (direction > 0) {
            if (ratio > 0.0) {
                if (_userClipPlanes[4] == NULL) {
                    _userClipPlanes[4] = vtkSmartPointer<vtkPlane>::New();
                    _userClipPlanes[4]->SetNormal(0, 0, 1);
                }
                _userClipPlanes[4]->SetOrigin(0, 0, bounds[4] + (bounds[5]-bounds[4])*ratio);
            } else {
                _userClipPlanes[4] = NULL;
            }
        } else {
            if (ratio < 1.0) {
                if (_userClipPlanes[5] == NULL) {
                    _userClipPlanes[5] = vtkSmartPointer<vtkPlane>::New();
                    _userClipPlanes[5]->SetNormal(0, 0, -1);
                }
                _userClipPlanes[5]->SetOrigin(0, 0, bounds[4] + (bounds[5]-bounds[4])*ratio);
            } else {
                _userClipPlanes[5] = NULL;
            }
        }
        break;
    default:
        ;
    }

    _needsRedraw = true;
}

/**
 * \brief Set up clipping planes for image camera mode if needed
 */
void Renderer::setCameraClippingPlanes()
{
    /* XXX: Note that there appears to be a bug with setting the 
     * clipping plane collection to NULL in the VTK Mappers -- 
     * the old clip planes are still applied.  The workaround here 
     * is to keep the PlaneCollection and add/remove the planes 
     * to/from the PlaneCollection as needed.
     */
    if (_cameraMode == IMAGE) {
        if (_activeClipPlanes->GetNumberOfItems() == 0) {
            for (int i = 0; i < 4; i++)
                _activeClipPlanes->AddItem(_cameraClipPlanes[i]);
        }
    } else {
        if (_activeClipPlanes->GetNumberOfItems() > 0)
            _activeClipPlanes->RemoveAllItems();
        for (int i = 0; i < 6; i++) {
            if (_userClipPlanes[i] != NULL) {
                _activeClipPlanes->AddItem(_userClipPlanes[i]);
            }
        }
    }

    /* Ensure all Mappers are using the PlaneCollection
     * This will not change the state or timestamp of 
     * Mappers already using the PlaneCollection
     */
    for (Contour2DHashmap::iterator itr = _contour2Ds.begin();
         itr != _contour2Ds.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (Contour3DHashmap::iterator itr = _contour3Ds.begin();
         itr != _contour3Ds.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (CutplaneHashmap::iterator itr = _cutplanes.begin();
         itr != _cutplanes.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (GlyphsHashmap::iterator itr = _glyphs.begin();
         itr != _glyphs.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (HeightMapHashmap::iterator itr = _heightMaps.begin();
         itr != _heightMaps.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (LICHashmap::iterator itr = _lics.begin();
         itr != _lics.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (MoleculeHashmap::iterator itr = _molecules.begin();
         itr != _molecules.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (PolyDataHashmap::iterator itr = _polyDatas.begin();
         itr != _polyDatas.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
         itr != _pseudoColors.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (StreamlinesHashmap::iterator itr = _streamlines.begin();
         itr != _streamlines.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
    for (VolumeHashmap::iterator itr = _volumes.begin();
         itr != _volumes.end(); ++itr) {
        itr->second->setClippingPlanes(_activeClipPlanes);
    }
}

/**
 * \brief Control the use of two sided lighting
 */
void Renderer::setUseTwoSidedLighting(bool state)
{
    _renderer->SetTwoSidedLighting(state ? 1 : 0);
    _needsRedraw = true;
}

/**
 * \brief Control the use of the depth peeling algorithm for transparency
 */
void Renderer::setUseDepthPeeling(bool state)
{
    _renderer->SetUseDepthPeeling(state ? 1 : 0);
    _needsRedraw = true;
}

/**
 * \brief Sets flag to trigger rendering next time render() is called
 */
void Renderer::eventuallyRender()
{
    _needsRedraw = true;
}

/**
 * \brief Cause the rendering to render a new image if needed
 *
 * The _needsRedraw flag indicates if a state change has occured since
 * the last rendered frame
 */
bool Renderer::render()
{
    if (_needsRedraw) {
        setCameraClippingPlanes();
        _renderWindow->Render();
        _needsRedraw = false;
        return true;
    } else
        return false;
}

/// Get the pixel width of the render window/image
int Renderer::getWindowWidth() const
{
    return _windowWidth;
}

/// Get the pixel height of the render window/image
int Renderer::getWindowHeight() const
{
    return _windowHeight;
}

/**
 * \brief Read back the rendered framebuffer image
 */
void Renderer::getRenderedFrame(vtkUnsignedCharArray *imgData)
{
#ifdef RENDER_TARGA
    _renderWindow->MakeCurrent();
    // Must clear previous errors first.
    while (glGetError() != GL_NO_ERROR){
        ;
    }
    int bytesPerPixel = TARGA_BYTES_PER_PIXEL;
    int size = bytesPerPixel * _windowWidth * _windowHeight;

    if (imgData->GetMaxId() + 1 != size)
    {
        imgData->SetNumberOfComponents(bytesPerPixel);
        imgData->SetNumberOfValues(size);
    }
    glDisable(GL_TEXTURE_2D);
    if (_renderWindow->GetDoubleBuffer()) {
        glReadBuffer(static_cast<GLenum>(vtkOpenGLRenderWindow::SafeDownCast(_renderWindow)->GetBackLeftBuffer()));
    } else {
        glReadBuffer(static_cast<GLenum>(vtkOpenGLRenderWindow::SafeDownCast(_renderWindow)->GetFrontLeftBuffer()));
    }
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
#ifdef WANT_TRACE
    struct timeval t1, t2;
    glFinish();
    gettimeofday(&t1, 0);
#endif
    if (bytesPerPixel == 4) {
        glReadPixels(0, 0, _windowWidth, _windowHeight, GL_BGRA,
                     GL_UNSIGNED_BYTE, imgData->GetPointer(0));
    } else {
        glReadPixels(0, 0, _windowWidth, _windowHeight, GL_BGR,
                     GL_UNSIGNED_BYTE, imgData->GetPointer(0));
    }
#ifdef WANT_TRACE
    gettimeofday(&t2, 0);
    static unsigned int numFrames = 0;
    static double accum = 0;
    numFrames++;
    accum += ELAPSED_TIME(t1, t2);
#endif
    TRACE("Readback time: %g ms", ELAPSED_TIME(t1, t2));
    TRACE("Readback avg: %g ms", accum/numFrames);
    if (glGetError() != GL_NO_ERROR) {
        ERROR("glReadPixels");
    }
#else
    _renderWindow->GetPixelData(0, 0, _windowWidth-1, _windowHeight-1,
                                !_renderWindow->GetDoubleBuffer(), imgData);
#endif
    TRACE("Image data size: %d", imgData->GetSize());
}

/**
 * \brief Get nearest data value given display coordinates x,y
 *
 * Note: no interpolation is performed on data
 */
bool Renderer::getScalarValueAtPixel(const DataSetId& id, int x, int y, double *value)
{
    vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
    coord->SetCoordinateSystemToDisplay();
    coord->SetValue(x, _windowHeight - y, 0);
    double *worldCoords = coord->GetComputedWorldValue(_renderer);

    TRACE("Pixel coords: %d, %d\nWorld coords: %g, %g, %g", x, y,
          worldCoords[0], 
          worldCoords[1], 
          worldCoords[2]);

    return getScalarValue(id, worldCoords[0], worldCoords[1], worldCoords[2], value);
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 */
bool Renderer::getScalarValue(const DataSetId& id, double x, double y, double z, double *value)
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return false;

    return ds->getScalarValue(x, y, z, value);
}

/**
 * \brief Get nearest data value given display coordinates x,y
 *
 * Note: no interpolation is performed on data
 */
bool Renderer::getVectorValueAtPixel(const DataSetId& id, int x, int y, double vector[3])
{
    vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
    coord->SetCoordinateSystemToDisplay();
    coord->SetValue(x, _windowHeight - y, 0);
    double *worldCoords = coord->GetComputedWorldValue(_renderer);

    TRACE("Pixel coords: %d, %d\nWorld coords: %g, %g, %g", x, y,
          worldCoords[0], 
          worldCoords[1], 
          worldCoords[2]);

    return getVectorValue(id, worldCoords[0], worldCoords[1], worldCoords[2], vector);
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 */
bool Renderer::getVectorValue(const DataSetId& id, double x, double y, double z, double vector[3])
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return false;

    return ds->getVectorValue(x, y, z, vector);
}
