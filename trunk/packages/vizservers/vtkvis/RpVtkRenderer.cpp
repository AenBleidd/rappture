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

#include "RpVtkRenderer.h"
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
    _cumulativeScalarRange[0] = 0.0;
    _cumulativeScalarRange[1] = 1.0;
    _cumulativeVectorMagnitudeRange[0] = 0.0;
    _cumulativeVectorMagnitudeRange[1] = 1.0;
    for (int i = 0; i < 3; i++) {
        _cumulativeVectorComponentRange[i][0] = 0.0;
        _cumulativeVectorComponentRange[i][1] = 1.0;
    }
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
 * \brief Remove the Contour2D isolines for the specified DataSet
 *
 * The underlying Contour2D is deleted, freeing its memory
 */
void Renderer::deleteContour2D(const DataSetId& id)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Contour2Ds for %s", id.c_str());

    do {
        Contour2D *contour = itr->second;
        if (contour->getProp())
            _renderer->RemoveViewProp(contour->getProp());
        delete contour;

        itr = _contour2Ds.erase(itr);
    } while (doAll && itr != _contour2Ds.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the Contour3D isosurfaces for the specified DataSet
 *
 * The underlying Contour3D is deleted, freeing its memory
 */
void Renderer::deleteContour3D(const DataSetId& id)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Contour3Ds for %s", id.c_str());

    do {
        Contour3D *contour = itr->second;
        if (contour->getProp())
            _renderer->RemoveViewProp(contour->getProp());
        delete contour;

        itr = _contour3Ds.erase(itr);
    } while (doAll && itr != _contour3Ds.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the Glyphs for the specified DataSet
 *
 * The underlying Glyphs is deleted, freeing its memory
 */
void Renderer::deleteGlyphs(const DataSetId& id)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Glyphs for %s", id.c_str());

    do {
        Glyphs *glyphs = itr->second;
        if (glyphs->getProp())
            _renderer->RemoveViewProp(glyphs->getProp());
        delete glyphs;

        itr = _glyphs.erase(itr);
    } while (doAll && itr != _glyphs.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the HeightMap for the specified DataSet
 *
 * The underlying HeightMap is deleted, freeing its memory
 */
void Renderer::deleteHeightMap(const DataSetId& id)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting HeightMaps for %s", id.c_str());

    do {
        HeightMap *hmap = itr->second;
        if (hmap->getProp())
            _renderer->RemoveViewProp(hmap->getProp());
        delete hmap;

        itr = _heightMaps.erase(itr);
    } while (doAll && itr != _heightMaps.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the LIC for the specified DataSet
 *
 * The underlying LIC is deleted, freeing its memory
 */
void Renderer::deleteLIC(const DataSetId& id)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting LICs for %s", id.c_str());

    do {
        LIC *lic = itr->second;
        if (lic->getProp())
            _renderer->RemoveViewProp(lic->getProp());
        delete lic;

        itr = _lics.erase(itr);
    } while (doAll && itr != _lics.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the Molecule for the specified DataSet
 *
 * The underlying Molecule is deleted, freeing its memory
 */
void Renderer::deleteMolecule(const DataSetId& id)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Molecules for %s", id.c_str());

    do {
        Molecule *molecule = itr->second;
        if (molecule->getProp())
            _renderer->RemoveViewProp(molecule->getProp());
        delete molecule;

        itr = _molecules.erase(itr);
    } while (doAll && itr != _molecules.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the PolyData mesh for the specified DataSet
 *
 * The underlying PolyData is deleted, freeing its memory
 */
void Renderer::deletePolyData(const DataSetId& id)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting PolyDatas for %s", id.c_str());

    do {
        PolyData *polyData = itr->second;
        if (polyData->getProp())
            _renderer->RemoveViewProp(polyData->getProp());
        delete polyData;

        itr = _polyDatas.erase(itr);
    } while (doAll && itr != _polyDatas.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the PseudoColor mapping for the specified DataSet
 *
 * The underlying PseudoColor object is deleted, freeing its memory
 */
void Renderer::deletePseudoColor(const DataSetId& id)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting PseudoColors for %s", id.c_str());

    do  {
        PseudoColor *ps = itr->second;
        if (ps->getProp())
            _renderer->RemoveViewProp(ps->getProp());
        delete ps;

        itr = _pseudoColors.erase(itr);
    } while (doAll && itr != _pseudoColors.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the Streamlines mapping for the specified DataSet
 *
 * The underlying Streamlines object is deleted, freeing its memory
 */
void Renderer::deleteStreamlines(const DataSetId& id)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Streamlines for %s", id.c_str());

    do  {
        Streamlines *sl = itr->second;
        if (sl->getProp())
            _renderer->RemoveViewProp(sl->getProp());
        delete sl;

        itr = _streamlines.erase(itr);
    } while (doAll && itr != _streamlines.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Remove the Volume for the specified DataSet
 *
 * The underlying Volume is deleted, freeing its memory
 */
void Renderer::deleteVolume(const DataSetId& id)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Volumes for %s", id.c_str());

    do {
        Volume *volume = itr->second;
        if (volume->getProp())
            _renderer->RemoveViewProp(volume->getProp());
        delete volume;

        itr = _volumes.erase(itr);
    } while (doAll && itr != _volumes.end());

    initCamera();
    _needsRedraw = true;
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

        deleteContour2D(itr->second->getName());
        deleteContour3D(itr->second->getName());
        deleteGlyphs(itr->second->getName());
        deleteHeightMap(itr->second->getName());
        deleteLIC(itr->second->getName());
        deleteMolecule(itr->second->getName());
        deletePolyData(itr->second->getName());
        deletePseudoColor(itr->second->getName());
        deleteStreamlines(itr->second->getName());
        deleteVolume(itr->second->getName());
 
        if (itr->second->getProp() != NULL) {
            _renderer->RemoveViewProp(itr->second->getProp());
        }

        TRACE("After deleting graphics objects");

        delete itr->second;
        itr = _dataSets.erase(itr);
    } while (doAll && itr != _dataSets.end());

    // Update cumulative data range
    updateRanges();

    initCamera();
    _needsRedraw = true;
}

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
        updateRanges();
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
        updateRanges();
        _needsRedraw = true;
        return ret;
    } else
        return false;
}

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
    do {
        if (!itr->second->setActiveScalars(scalarName)) {
            ret = false;
        }
    } while (doAll && ++itr != _dataSets.end());

    if (ret) {
         updateRanges();
        _needsRedraw = true;
    } 

    return ret;
}

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
    do {
        if (!itr->second->setActiveVectors(vectorName)) {
            ret = false;
        }
    } while (doAll && ++itr != _dataSets.end());

    if (ret) {
        updateRanges();
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
        updateRanges();
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
            _cubeAxesActor->SetBounds(newBounds);
            TRACE("Bounds (computed): %g %g %g %g %g %g",
                  newBounds[0],
                  newBounds[1],
                  newBounds[2],
                  newBounds[3],
                  newBounds[4],
                  newBounds[5]);
        } else {
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
    _cubeAxesActor->SetFlyModeToClosestTriad();

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
 * \param[in] id ColorMap name
 * \param[in] dataSetID DataSet name
 * \param[in] legendType scalar or vector field legend
 * \param[in,out] title If supplied, draw title ("#auto" means to
 * fill in field name and draw).  If blank, do not draw title.  
 * If title was blank or "#auto", will be filled with field name on
 * return
 * \param[out] range Filled with min and max values
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
    TRACE("Enter");
    ColorMap *colorMap = getColorMap(id);
    if (colorMap == NULL)
        return false;

    if (_legendRenderWindow == NULL) {
        _legendRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
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

    range[0] = 0.0;
    range[1] = 1.0;

    switch (legendType) {
    case ACTIVE_VECTOR_MAGNITUDE:
        if (cumulative) {
            lut->SetRange(_cumulativeVectorMagnitudeRange);
            range[0] = _cumulativeVectorMagnitudeRange[0];
            range[1] = _cumulativeVectorMagnitudeRange[1];
        } else if (dataSet != NULL) {
            dataSet->getVectorRange(range);
            lut->SetRange(range);
        }
        if (title.empty() && dataSet != NULL) {
            const char *name = dataSet->getActiveVectorsName();
            if (name != NULL) {
                title = name;
                title.append("(mag)");
            }
        }
        break;
    case ACTIVE_VECTOR_X:
        if (cumulative) {
            lut->SetRange(_cumulativeVectorComponentRange[0]);
            range[0] = _cumulativeVectorComponentRange[0][0];
            range[1] = _cumulativeVectorComponentRange[0][1];
        } else if (dataSet != NULL) {
            dataSet->getVectorRange(range, 0);
            lut->SetRange(range);
        }
        if (title.empty() && dataSet != NULL) {
            const char *name = dataSet->getActiveVectorsName();
            if (name != NULL) {
                title = name;
                title.append("(x)");
            }
        }
        break;
    case ACTIVE_VECTOR_Y:
        if (cumulative) {
            lut->SetRange(_cumulativeVectorComponentRange[1]);
            range[0] = _cumulativeVectorComponentRange[1][0];
            range[1] = _cumulativeVectorComponentRange[1][1];
        } else if (dataSet != NULL) {
            dataSet->getVectorRange(range, 1);
            lut->SetRange(range);
        }
        if (title.empty() && dataSet != NULL) {
            const char *name = dataSet->getActiveVectorsName();
            if (name != NULL) {
                title = name;
                title.append("(y)");
            }
        }
        break;
    case ACTIVE_VECTOR_Z:
        if (cumulative) {
            lut->SetRange(_cumulativeVectorComponentRange[2]);
            range[0] = _cumulativeVectorComponentRange[2][0];
            range[1] = _cumulativeVectorComponentRange[2][1];
        } else if (dataSet != NULL) {
            dataSet->getVectorRange(range, 1);
            lut->SetRange(range);
        }
        if (title.empty() && dataSet != NULL) {
            const char *name = dataSet->getActiveVectorsName();
            if (name != NULL) {
                title = name;
                title.append("(z)");
            }
        }
        break;
    case ACTIVE_SCALAR:
    default:
        if (cumulative) {
            lut->SetRange(_cumulativeScalarRange);
            range[0] = _cumulativeScalarRange[0];
            range[1] = _cumulativeScalarRange[1];
        } else if (dataSet != NULL) {
            dataSet->getScalarRange(range);
            lut->SetRange(range);
        }
        if (title.empty() && dataSet != NULL) {
            const char *name = dataSet->getActiveScalarsName();
            if (name != NULL)
                title = name;
        }
        break;
    }

    _scalarBarActor->SetLookupTable(lut);

    // Set viewport-relative width/height/pos
    if (width > height) {
        _scalarBarActor->SetOrientationToHorizontal();
        _scalarBarActor->SetHeight(0.8);
        _scalarBarActor->SetWidth(0.85);
        _scalarBarActor->SetPosition(.075, .1);
    } else {
        _scalarBarActor->SetOrientationToVertical();
        _scalarBarActor->SetHeight(0.9);
        _scalarBarActor->SetWidth(0.8);
        _scalarBarActor->SetPosition(.1, .05);
    }
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
    glReadBuffer(static_cast<GLenum>(vtkOpenGLRenderWindow::SafeDownCast(_renderWindow)->GetFrontLeftBuffer()));
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
 * \brief Create a new Contour2D and associate it with the named DataSet
 */
bool Renderer::addContour2D(const DataSetId& id, int numContours)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getContour2D(dsID)) {
            WARN("Replacing existing Contour2D %s", dsID.c_str());
            deleteContour2D(dsID);
        }

        Contour2D *contour = new Contour2D(numContours);
 
        contour->setDataSet(ds,
                            _useCumulativeRange, 
                            _cumulativeScalarRange,
                            _cumulativeVectorMagnitudeRange,
                            _cumulativeVectorComponentRange);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour2Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new Contour2D and associate it with the named DataSet
 */
bool Renderer::addContour2D(const DataSetId& id, const std::vector<double>& contours)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getContour2D(dsID)) {
            WARN("Replacing existing Contour2D %s", dsID.c_str());
            deleteContour2D(dsID);
        }

        Contour2D *contour = new Contour2D(contours);

        contour->setDataSet(ds,
                            _useCumulativeRange, 
                            _cumulativeScalarRange,
                            _cumulativeVectorMagnitudeRange,
                            _cumulativeVectorComponentRange);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour2Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the Contour2D associated with a named DataSet
 */
Contour2D *Renderer::getContour2D(const DataSetId& id)
{
    Contour2DHashmap::iterator itr = _contour2Ds.find(id);

    if (itr == _contour2Ds.end()) {
#ifdef DEBUG
        TRACE("Contour2D not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setContour2DTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _contour2Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setContour2DOrientation(const DataSetId& id, double quat[4])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _contour2Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setContour2DOrientation(const DataSetId& id, double angle, double axis[3])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _contour2Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setContour2DPosition(const DataSetId& id, double pos[3])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _contour2Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setContour2DScale(const DataSetId& id, double scale[3])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _contour2Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the number of equally spaced contour isolines for the given DataSet
 */
void Renderer::setContour2DContours(const DataSetId& id, int numContours)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContours(numContours);
    } while (doAll && ++itr != _contour2Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set a list of isovalues for the given DataSet
 */
void Renderer::setContour2DContourList(const DataSetId& id, const std::vector<double>& contours)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourList(contours);
    } while (doAll && ++itr != _contour2Ds.end());

     _needsRedraw = true;
}

/**
 * \brief Set opacity of contour lines for the given DataSet
 */
void Renderer::setContour2DOpacity(const DataSetId& id, double opacity)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _contour2Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering contour lines for the given DataSet
 */
void Renderer::setContour2DVisibility(const DataSetId& id, bool state)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _contour2Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB isoline color for the specified DataSet
 */
void Renderer::setContour2DColor(const DataSetId& id, float color[3])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColor(color);
    } while (doAll && ++itr != _contour2Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the isoline width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setContour2DEdgeWidth(const DataSetId& id, float edgeWidth)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _contour2Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Turn contour lighting on/off for the specified DataSet
 */
void Renderer::setContour2DLighting(const DataSetId& id, bool state)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _contour2Ds.end());
    _needsRedraw = true;
}

/**
 * \brief Create a new Contour3D and associate it with the named DataSet
 */
bool Renderer::addContour3D(const DataSetId& id, int numContours)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getContour3D(dsID)) {
            WARN("Replacing existing Contour3D %s", dsID.c_str());
            deleteContour3D(dsID);
        }

        Contour3D *contour = new Contour3D(numContours);

        contour->setDataSet(ds,
                            _useCumulativeRange,
                            _cumulativeScalarRange,
                            _cumulativeVectorMagnitudeRange,
                            _cumulativeVectorComponentRange);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour3Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new Contour3D and associate it with the named DataSet
 */
bool Renderer::addContour3D(const DataSetId& id,const std::vector<double>& contours)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getContour3D(dsID)) {
            WARN("Replacing existing Contour3D %s", dsID.c_str());
            deleteContour3D(dsID);
        }

        Contour3D *contour = new Contour3D(contours);

        contour->setDataSet(ds,
                            _useCumulativeRange, 
                            _cumulativeScalarRange,
                            _cumulativeVectorMagnitudeRange,
                            _cumulativeVectorComponentRange);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour3Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the Contour3D associated with a named DataSet
 */
Contour3D *Renderer::getContour3D(const DataSetId& id)
{
    Contour3DHashmap::iterator itr = _contour3Ds.find(id);

    if (itr == _contour3Ds.end()) {
#ifdef DEBUG
        TRACE("Contour3D not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setContour3DTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _contour3Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setContour3DOrientation(const DataSetId& id, double quat[4])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _contour3Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setContour3DOrientation(const DataSetId& id, double angle, double axis[3])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _contour3Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setContour3DPosition(const DataSetId& id, double pos[3])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _contour3Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setContour3DScale(const DataSetId& id, double scale[3])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _contour3Ds.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the number of equally spaced isosurfaces for the given DataSet
 */
void Renderer::setContour3DContours(const DataSetId& id, int numContours)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContours(numContours);
     } while (doAll && ++itr != _contour3Ds.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Set a list of isovalues for the given DataSet
 */
void Renderer::setContour3DContourList(const DataSetId& id, const std::vector<double>& contours)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourList(contours);
    } while (doAll && ++itr != _contour3Ds.end());

    initCamera();
     _needsRedraw = true;
}

/**
 * \brief Set opacity of isosurfaces for the given DataSet
 */
void Renderer::setContour3DOpacity(const DataSetId& id, double opacity)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering isosurfaces for the given DataSet
 */
void Renderer::setContour3DVisibility(const DataSetId& id, bool state)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a Contour3D for the given 
 * DataSet
 */
void Renderer::setContour3DColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }

    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set Contour3D color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB isosurface color for the specified DataSet
 */
void Renderer::setContour3DColor(const DataSetId& id, float color[3])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColor(color);
    } while (doAll && ++itr != _contour3Ds.end());
    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering isosurface edges for the given DataSet
 */
void Renderer::setContour3DEdgeVisibility(const DataSetId& id, bool state)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB isosurface edge color for the specified DataSet
 */
void Renderer::setContour3DEdgeColor(const DataSetId& id, float color[3])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the isosurface edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setContour3DEdgeWidth(const DataSetId& id, float edgeWidth)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set wireframe rendering for the specified DataSet
 */
void Renderer::setContour3DWireframe(const DataSetId& id, bool state)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWireframe(state);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Turn contour lighting on/off for the specified DataSet
 */
void Renderer::setContour3DLighting(const DataSetId& id, bool state)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _contour3Ds.end());
    _needsRedraw = true;
}

/**
 * \brief Create a new Glyphs and associate it with the named DataSet
 */
bool Renderer::addGlyphs(const DataSetId& id, Glyphs::GlyphShape shape)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getGlyphs(dsID)) {
            WARN("Replacing existing Glyphs %s", dsID.c_str());
            deleteGlyphs(dsID);
        }

        Glyphs *glyphs = new Glyphs(shape);

        glyphs->setDataSet(ds,
                           _useCumulativeRange, 
                           _cumulativeScalarRange,
                           _cumulativeVectorMagnitudeRange,
                           _cumulativeVectorComponentRange);

        if (glyphs->getProp() == NULL) {
            delete glyphs;
            return false;
        } else {
            _renderer->AddViewProp(glyphs->getProp());
        }

        _glyphs[dsID] = glyphs;
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();

    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the Glyphs associated with a named DataSet
 */
Glyphs *Renderer::getGlyphs(const DataSetId& id)
{
    GlyphsHashmap::iterator itr = _glyphs.find(id);

    if (itr == _glyphs.end()) {
#ifdef DEBUG
        TRACE("Glyphs not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setGlyphsTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _glyphs.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setGlyphsOrientation(const DataSetId& id, double quat[4])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _glyphs.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setGlyphsOrientation(const DataSetId& id, double angle, double axis[3])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _glyphs.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setGlyphsPosition(const DataSetId& id, double pos[3])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _glyphs.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setGlyphsScale(const DataSetId& id, double scale[3])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _glyphs.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the RGB polygon color for the specified DataSet
 */
void Renderer::setGlyphsColor(const DataSetId& id, float color[3])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColor(color);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a Glyphs for the given DataSet
 */
void Renderer::setGlyphsColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }

    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set Glyphs color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Controls the array used to color glyphs for the given DataSet
 */
void Renderer::setGlyphsColorMode(const DataSetId& id, Glyphs::ColorMode mode)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Controls the array used to scale glyphs for the given DataSet
 */
void Renderer::setGlyphsScalingMode(const DataSetId& id, Glyphs::ScalingMode mode)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScalingMode(mode);
    } while (doAll && ++itr != _glyphs.end());

    _renderer->ResetCameraClippingRange();
    _needsRedraw = true;
}

/**
 * \brief Set the shape of Glyphs for the given DataSet
 */
void Renderer::setGlyphsShape(const DataSetId& id, Glyphs::GlyphShape shape)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setGlyphShape(shape);
    } while (doAll && ++itr != _glyphs.end());

    _renderer->ResetCameraClippingRange();
    _needsRedraw = true;
}

/**
 * \brief Set the glyph scaling factor for the given DataSet
 */
void Renderer::setGlyphsScaleFactor(const DataSetId& id, double scale)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScaleFactor(scale);
    } while (doAll && ++itr != _glyphs.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setGlyphsEdgeVisibility(const DataSetId& id, bool state)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB polygon edge color for the specified DataSet
 */
void Renderer::setGlyphsEdgeColor(const DataSetId& id, float color[3])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Set the polygon edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setGlyphsEdgeWidth(const DataSetId& id, float edgeWidth)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Turn Glyphs lighting on/off for the specified DataSet
 */
void Renderer::setGlyphsLighting(const DataSetId& id, bool state)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _glyphs.end());
    _needsRedraw = true;
}

/**
 * \brief Set opacity of Glyphs for the given DataSet
 */
void Renderer::setGlyphsOpacity(const DataSetId& id, double opacity)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering Glyphs for the given DataSet
 */
void Renderer::setGlyphsVisibility(const DataSetId& id, bool state)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off wireframe rendering of Glyphs for the given DataSet
 */
void Renderer::setGlyphsWireframe(const DataSetId& id, bool state)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWireframe(state);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new HeightMap and associate it with the named DataSet
 */
bool Renderer::addHeightMap(const DataSetId& id, int numContours, double heightScale)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getHeightMap(dsID)) {
            WARN("Replacing existing HeightMap %s", dsID.c_str());
            deleteHeightMap(dsID);
        }

        HeightMap *hmap = new HeightMap(numContours, heightScale);

        hmap->setDataSet(ds,
                         _useCumulativeRange, 
                         _cumulativeScalarRange,
                         _cumulativeVectorMagnitudeRange,
                         _cumulativeVectorComponentRange);

        if (hmap->getProp() == NULL) {
            delete hmap;
            return false;
        } else {
            _renderer->AddViewProp(hmap->getProp());
        }

        _heightMaps[dsID] = hmap;
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();

    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new HeightMap and associate it with the named DataSet
 */
bool Renderer::addHeightMap(const DataSetId& id, const std::vector<double>& contours, double heightScale)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getHeightMap(dsID)) {
            WARN("Replacing existing HeightMap %s", dsID.c_str());
            deleteHeightMap(dsID);
        }

        HeightMap *hmap = new HeightMap(contours, heightScale);

        hmap->setDataSet(ds,
                         _useCumulativeRange, 
                         _cumulativeScalarRange,
                         _cumulativeVectorMagnitudeRange,
                         _cumulativeVectorComponentRange);

        if (hmap->getProp() == NULL) {
            delete hmap;
            return false;
        } else {
            _renderer->AddViewProp(hmap->getProp());
        }

        _heightMaps[dsID] = hmap;
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();

    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the HeightMap associated with a named DataSet
 */
HeightMap *Renderer::getHeightMap(const DataSetId& id)
{
    HeightMapHashmap::iterator itr = _heightMaps.find(id);

    if (itr == _heightMaps.end()) {
#ifdef DEBUG
        TRACE("HeightMap not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set an additional transform on the prop
 */
void Renderer::setHeightMapTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _heightMaps.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setHeightMapOrientation(const DataSetId& id, double quat[4])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _heightMaps.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setHeightMapOrientation(const DataSetId& id, double angle, double axis[3])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _heightMaps.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setHeightMapPosition(const DataSetId& id, double pos[3])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _heightMaps.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setHeightMapScale(const DataSetId& id, double scale[3])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _heightMaps.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the volume slice used for mapping volumetric data
 */
void Renderer::setHeightMapVolumeSlice(const DataSetId& id, HeightMap::Axis axis, double ratio)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }

    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->selectVolumeSlice(axis, ratio);
     } while (doAll && ++itr != _heightMaps.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Set amount to scale scalar values when creating elevations
 * in the height map
 */
void Renderer::setHeightMapHeightScale(const DataSetId& id, double scale)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }

    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setHeightScale(scale);
     } while (doAll && ++itr != _heightMaps.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a HeightMap for the given DataSet
 */
void Renderer::setHeightMapColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }

    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set HeightMap color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the number of equally spaced contour isolines for the given DataSet
 */
void Renderer::setHeightMapNumContours(const DataSetId& id, int numContours)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setNumContours(numContours);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set a list of height map contour isovalues for the given DataSet
 */
void Renderer::setHeightMapContourList(const DataSetId& id, const std::vector<double>& contours)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourList(contours);
    } while (doAll && ++itr != _heightMaps.end());

     _needsRedraw = true;
}

/**
 * \brief Set opacity of height map for the given DataSet
 */
void Renderer::setHeightMapOpacity(const DataSetId& id, double opacity)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering height map for the given DataSet
 */
void Renderer::setHeightMapVisibility(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set wireframe rendering for the specified DataSet
 */
void Renderer::setHeightMapWireframe(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWireframe(state);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering height map mesh edges for the given DataSet
 */
void Renderer::setHeightMapEdgeVisibility(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB height map mesh edge color for the specified DataSet
 */
void Renderer::setHeightMapEdgeColor(const DataSetId& id, float color[3])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the height map mesh edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setHeightMapEdgeWidth(const DataSetId& id, float edgeWidth)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering height map contour lines for the given DataSet
 */
void Renderer::setHeightMapContourLineVisibility(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourLineVisibility(state);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering height map colormap surface for the given DataSet
 */
void Renderer::setHeightMapContourSurfaceVisibility(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourSurfaceVisibility(state);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB height map isoline color for the specified DataSet
 */
void Renderer::setHeightMapContourEdgeColor(const DataSetId& id, float color[3])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourEdgeColor(color);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the height map isoline width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setHeightMapContourEdgeWidth(const DataSetId& id, float edgeWidth)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Turn height map lighting on/off for the specified DataSet
 */
void Renderer::setHeightMapLighting(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _heightMaps.end());
    _needsRedraw = true;
}

/**
 * \brief Create a new LIC and associate it with the named DataSet
 */
bool Renderer::addLIC(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getLIC(dsID)) {
            WARN("Replacing existing LIC %s", dsID.c_str());
            deleteLIC(dsID);
        }

        LIC *lic = new LIC();
        _lics[dsID] = lic;

        lic->setDataSet(ds,
                        _useCumulativeRange, 
                        _cumulativeScalarRange,
                        _cumulativeVectorMagnitudeRange,
                        _cumulativeVectorComponentRange);

        _renderer->AddViewProp(lic->getProp());
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the LIC associated with a named DataSet
 */
LIC *Renderer::getLIC(const DataSetId& id)
{
    LICHashmap::iterator itr = _lics.find(id);

    if (itr == _lics.end()) {
#ifdef DEBUG
        TRACE("LIC not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setLICTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _lics.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setLICOrientation(const DataSetId& id, double quat[4])
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _lics.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setLICOrientation(const DataSetId& id, double angle, double axis[3])
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _lics.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setLICPosition(const DataSetId& id, double pos[3])
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _lics.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setLICScale(const DataSetId& id, double scale[3])
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _lics.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the volume slice used for mapping volumetric data
 */
void Renderer::setLICVolumeSlice(const DataSetId& id, LIC::Axis axis, double ratio)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }

    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->selectVolumeSlice(axis, ratio);
     } while (doAll && ++itr != _lics.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with an LIC for the given DataSet
 */
void Renderer::setLICColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }

    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set LIC color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _lics.end());

    _needsRedraw = true;
}

/**
 * \brief Set opacity of the LIC for the given DataSet
 */
void Renderer::setLICOpacity(const DataSetId& id, double opacity)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _lics.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the LIC for the given DataSet
 */
void Renderer::setLICVisibility(const DataSetId& id, bool state)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _lics.end());

    _needsRedraw = true;
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setLICEdgeVisibility(const DataSetId& id, bool state)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _lics.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB polygon edge color for the specified DataSet
 */
void Renderer::setLICEdgeColor(const DataSetId& id, float color[3])
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _lics.end());

    _needsRedraw = true;
}

/**
 * \brief Set the polygon edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setLICEdgeWidth(const DataSetId& id, float edgeWidth)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _lics.end());

    _needsRedraw = true;
}

/**
 * \brief Turn LIC lighting on/off for the specified DataSet
 */
void Renderer::setLICLighting(const DataSetId& id, bool state)
{
    LICHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _lics.begin();
        doAll = true;
    } else {
        itr = _lics.find(id);
    }
    if (itr == _lics.end()) {
        ERROR("LIC not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _lics.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Molecule and associate it with the named DataSet
 */
bool Renderer::addMolecule(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getMolecule(dsID)) {
            WARN("Replacing existing Molecule %s", dsID.c_str());
            deleteMolecule(dsID);
        }

        Molecule *molecule = new Molecule();
        _molecules[dsID] = molecule;

        molecule->setDataSet(ds);

        _renderer->AddViewProp(molecule->getProp());
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the Molecule associated with a named DataSet
 */
Molecule *Renderer::getMolecule(const DataSetId& id)
{
    MoleculeHashmap::iterator itr = _molecules.find(id);

    if (itr == _molecules.end()) {
#ifdef DEBUG
        TRACE("Molecule not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setMoleculeTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _molecules.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setMoleculeOrientation(const DataSetId& id, double quat[4])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _molecules.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setMoleculeOrientation(const DataSetId& id, double angle, double axis[3])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _molecules.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setMoleculePosition(const DataSetId& id, double pos[3])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _molecules.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setMoleculeScale(const DataSetId& id, double scale[3])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _molecules.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a Molecule for the given DataSet
 */
void Renderer::setMoleculeColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }

    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set Molecule color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set opacity of the Molecule for the given DataSet
 */
void Renderer::setMoleculeOpacity(const DataSetId& id, double opacity)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set radius standard for scaling atoms
 */
void Renderer::setMoleculeAtomScaling(const DataSetId& id, Molecule::AtomScaling scaling)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomScaling(scaling);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Molecule atoms for the given DataSet
 */
void Renderer::setMoleculeAtomVisibility(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomVisibility(state);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Molecule bonds for the given DataSet
 */
void Renderer::setMoleculeBondVisibility(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBondVisibility(state);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Molecule for the given DataSet
 */
void Renderer::setMoleculeVisibility(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setMoleculeEdgeVisibility(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB polygon edge color for the specified DataSet
 */
void Renderer::setMoleculeEdgeColor(const DataSetId& id, float color[3])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set the polygon edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setMoleculeEdgeWidth(const DataSetId& id, float edgeWidth)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set wireframe rendering for the specified DataSet
 */
void Renderer::setMoleculeWireframe(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWireframe(state);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Turn Molecule lighting on/off for the specified DataSet
 */
void Renderer::setMoleculeLighting(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new PolyData and associate it with the named DataSet
 */
bool Renderer::addPolyData(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getPolyData(dsID)) {
            WARN("Replacing existing PolyData %s", dsID.c_str());
            deletePolyData(dsID);
        }

        PolyData *polyData = new PolyData();
        _polyDatas[dsID] = polyData;

        polyData->setDataSet(ds);

        _renderer->AddViewProp(polyData->getProp());
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the PolyData associated with a named DataSet
 */
PolyData *Renderer::getPolyData(const DataSetId& id)
{
    PolyDataHashmap::iterator itr = _polyDatas.find(id);

    if (itr == _polyDatas.end()) {
#ifdef DEBUG
        TRACE("PolyData not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set an additional transform on the prop
 */
void Renderer::setPolyDataTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _polyDatas.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setPolyDataOrientation(const DataSetId& id, double quat[4])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _polyDatas.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setPolyDataOrientation(const DataSetId& id, double angle, double axis[3])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _polyDatas.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setPolyDataPosition(const DataSetId& id, double pos[3])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _polyDatas.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setPolyDataScale(const DataSetId& id, double scale[3])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _polyDatas.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set opacity of the PolyData for the given DataSet
 */
void Renderer::setPolyDataOpacity(const DataSetId& id, double opacity)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the PolyData mapper for the given DataSet
 */
void Renderer::setPolyDataVisibility(const DataSetId& id, bool state)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB polygon face color for the specified DataSet
 */
void Renderer::setPolyDataColor(const DataSetId& id, float color[3])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColor(color);
    } while (doAll && ++itr != _polyDatas.end());
    _needsRedraw = true;
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setPolyDataEdgeVisibility(const DataSetId& id, bool state)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB polygon edge color for the specified DataSet
 */
void Renderer::setPolyDataEdgeColor(const DataSetId& id, float color[3])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set the polygon edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setPolyDataEdgeWidth(const DataSetId& id, float edgeWidth)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set the point size for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide points, 
 * this function may not have an effect.
 */
void Renderer::setPolyDataPointSize(const DataSetId& id, float size)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPointSize(size);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set wireframe rendering for the specified DataSet
 */
void Renderer::setPolyDataWireframe(const DataSetId& id, bool state)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWireframe(state);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Turn mesh lighting on/off for the specified DataSet
 */
void Renderer::setPolyDataLighting(const DataSetId& id, bool state)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new PseudoColor rendering for the specified DataSet
 */
bool Renderer::addPseudoColor(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getPseudoColor(dsID)) {
            WARN("Replacing existing PseudoColor %s", dsID.c_str());
            deletePseudoColor(dsID);
        }
        PseudoColor *pc = new PseudoColor();
        _pseudoColors[dsID] = pc;

        pc->setDataSet(ds,
                       _useCumulativeRange, 
                       _cumulativeScalarRange,
                       _cumulativeVectorMagnitudeRange,
                       _cumulativeVectorComponentRange);

        _renderer->AddViewProp(pc->getProp());
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the PseudoColor associated with the specified DataSet
 */
PseudoColor *Renderer::getPseudoColor(const DataSetId& id)
{
    PseudoColorHashmap::iterator itr = _pseudoColors.find(id);

    if (itr == _pseudoColors.end()) {
#ifdef DEBUG
        TRACE("PseudoColor not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setPseudoColorTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _pseudoColors.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setPseudoColorOrientation(const DataSetId& id, double quat[4])
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _pseudoColors.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setPseudoColorOrientation(const DataSetId& id, double angle, double axis[3])
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _pseudoColors.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setPseudoColorPosition(const DataSetId& id, double pos[3])
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _pseudoColors.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setPseudoColorScale(const DataSetId& id, double scale[3])
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _pseudoColors.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a PseudoColor for the given DataSet
 */
void Renderer::setPseudoColorColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }

    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set PseudoColor color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Set opacity of the PseudoColor for the given DataSet
 */
void Renderer::setPseudoColorOpacity(const DataSetId& id, double opacity)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }

    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the PseudoColor mapper for the given DataSet
 */
void Renderer::setPseudoColorVisibility(const DataSetId& id, bool state)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }

    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setPseudoColorEdgeVisibility(const DataSetId& id, bool state)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }

    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB polygon edge color for the specified DataSet
 */
void Renderer::setPseudoColorEdgeColor(const DataSetId& id, float color[3])
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }

    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Set the polygon edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setPseudoColorEdgeWidth(const DataSetId& id, float edgeWidth)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }

    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Set wireframe rendering for the specified DataSet
 */
void Renderer::setPseudoColorWireframe(const DataSetId& id, bool state)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWireframe(state);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Turn mesh lighting on/off for the specified DataSet
 */
void Renderer::setPseudoColorLighting(const DataSetId& id, bool state)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }

    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Streamlines and associate it with the named DataSet
 */
bool Renderer::addStreamlines(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getStreamlines(dsID)) {
            WARN("Replacing existing Streamlines %s", dsID.c_str());
            deleteStreamlines(dsID);
        }

        Streamlines *streamlines = new Streamlines();
        _streamlines[dsID] = streamlines;

        streamlines->setDataSet(ds,
                                _useCumulativeRange, 
                                _cumulativeScalarRange,
                                _cumulativeVectorMagnitudeRange,
                                _cumulativeVectorComponentRange);

        _renderer->AddViewProp(streamlines->getProp());
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the Streamlines associated with a named DataSet
 */
Streamlines *Renderer::getStreamlines(const DataSetId& id)
{
    StreamlinesHashmap::iterator itr = _streamlines.find(id);

    if (itr == _streamlines.end()) {
#ifdef DEBUG
        TRACE("Streamlines not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setStreamlinesTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _streamlines.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setStreamlinesOrientation(const DataSetId& id, double quat[4])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _streamlines.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setStreamlinesOrientation(const DataSetId& id, double angle, double axis[3])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _streamlines.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setStreamlinesPosition(const DataSetId& id, double pos[3])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _streamlines.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setStreamlinesScale(const DataSetId& id, double scale[3])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _streamlines.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points distributed randomly inside
 * cells of DataSet
 */
void Renderer::setStreamlinesSeedToRandomPoints(const DataSetId& id, int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToRandomPoints(numPoints);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points along a line
 */
void Renderer::setStreamlinesSeedToRake(const DataSetId& id,
                                        double start[3], double end[3],
                                        int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToRake(start, end, numPoints);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points inside a disk, with optional
 * hole
 */
void Renderer::setStreamlinesSeedToDisk(const DataSetId& id,
                                        double center[3], double normal[3],
                                        double radius, double innerRadius,
                                        int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToDisk(center, normal, radius, innerRadius, numPoints);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to vertices of an n-sided polygon
 */
void Renderer::setStreamlinesSeedToPolygon(const DataSetId& id,
                                           double center[3], double normal[3],
                                           double angle, double radius,
                                           int numSides)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToPolygon(center, normal, angle, radius, numSides);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to vertices of an n-sided polygon
 */
void Renderer::setStreamlinesSeedToFilledPolygon(const DataSetId& id,
                                                 double center[3],
                                                 double normal[3],
                                                 double angle, double radius,
                                                 int numSides, int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToFilledPolygon(center, normal, angle,
                                            radius, numSides, numPoints);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set Streamlines rendering to polylines for the specified DataSet
 */
void Renderer::setStreamlinesTypeToLines(const DataSetId& id)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLineTypeToLines();
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set Streamlines rendering to tubes for the specified DataSet
 */
void Renderer::setStreamlinesTypeToTubes(const DataSetId& id, int numSides, double radius)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLineTypeToTubes(numSides, radius);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set Streamlines rendering to ribbons for the specified DataSet
 */
void Renderer::setStreamlinesTypeToRibbons(const DataSetId& id, double width, double angle)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLineTypeToRibbons(width, angle);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set Streamlines maximum length for the specified DataSet
 */
void Renderer::setStreamlinesLength(const DataSetId& id, double length)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setMaxPropagation(length);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set Streamlines opacity scaling for the specified DataSet
 */
void Renderer::setStreamlinesOpacity(const DataSetId& id, double opacity)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Streamlines mapper for the given DataSet
 */
void Renderer::setStreamlinesVisibility(const DataSetId& id, bool state)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Streamlines seed geometry for the given DataSet
 */
void Renderer::setStreamlinesSeedVisibility(const DataSetId& id, bool state)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedVisibility(state);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setStreamlinesColorMode(const DataSetId& id, Streamlines::ColorMode mode)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a Streamlines for the given DataSet
 */
void Renderer::setStreamlinesColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }

    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set Streamlines color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB line/edge color for the specified DataSet
 */
void Renderer::setStreamlinesSeedColor(const DataSetId& id, float color[3])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedColor(color);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setStreamlinesEdgeVisibility(const DataSetId& id, bool state)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }

    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB color for the specified DataSet
 */
void Renderer::setStreamlinesColor(const DataSetId& id, float color[3])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColor(color);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB line/edge color for the specified DataSet
 */
void Renderer::setStreamlinesEdgeColor(const DataSetId& id, float color[3])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the line/edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setStreamlinesEdgeWidth(const DataSetId& id, float edgeWidth)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Turn Streamlines lighting on/off for the specified DataSet
 */
void Renderer::setStreamlinesLighting(const DataSetId& id, bool state)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Volume and associate it with the named DataSet
 */
bool Renderer::addVolume(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getVolume(dsID)) {
            WARN("Replacing existing Volume %s", dsID.c_str());
            deleteVolume(dsID);
        }

        Volume *volume = new Volume();
        _volumes[dsID] = volume;

        volume->setDataSet(ds,
                           _useCumulativeRange, 
                           _cumulativeScalarRange,
                           _cumulativeVectorMagnitudeRange,
                           _cumulativeVectorComponentRange );

        _renderer->AddViewProp(volume->getProp());
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Get the Volume associated with a named DataSet
 */
Volume *Renderer::getVolume(const DataSetId& id)
{
    VolumeHashmap::iterator itr = _volumes.find(id);

    if (itr == _volumes.end()) {
#ifdef DEBUG
        TRACE("Volume not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setVolumeTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != _volumes.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
void Renderer::setVolumeOrientation(const DataSetId& id, double quat[4])
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != _volumes.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
void Renderer::setVolumeOrientation(const DataSetId& id, double angle, double axis[3])
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != _volumes.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
void Renderer::setVolumePosition(const DataSetId& id, double pos[3])
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != _volumes.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
void Renderer::setVolumeScale(const DataSetId& id, double scale[3])
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != _volumes.end());

    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a Volume for the given DataSet
 */
void Renderer::setVolumeColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }

    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        TRACE("Set Volume color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Set Volume opacity scaling for the specified DataSet
 */
void Renderer::setVolumeOpacity(const DataSetId& id, double opacity)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Volume mapper for the given DataSet
 */
void Renderer::setVolumeVisibility(const DataSetId& id, bool state)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Set volume ambient lighting/shading coefficient for the specified DataSet
 */
void Renderer::setVolumeAmbient(const DataSetId& id, double coeff)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAmbient(coeff);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Set volume diffuse lighting/shading coefficient for the specified DataSet
 */
void Renderer::setVolumeDiffuse(const DataSetId& id, double coeff)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setDiffuse(coeff);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Set volume specular lighting/shading coefficient and power for the specified DataSet
 */
void Renderer::setVolumeSpecular(const DataSetId& id, double coeff, double power)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSpecular(coeff, power);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Turn volume lighting/shading on/off for the specified DataSet
 */
void Renderer::setVolumeLighting(const DataSetId& id, bool state)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
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

inline void quaternionToMatrix4x4(const double quat[4], vtkMatrix4x4& mat)
{
    double ww = quat[0]*quat[0];
    double wx = quat[0]*quat[1];
    double wy = quat[0]*quat[2];
    double wz = quat[0]*quat[3];

    double xx = quat[1]*quat[1];
    double yy = quat[2]*quat[2];
    double zz = quat[3]*quat[3];

    double xy = quat[1]*quat[2];
    double xz = quat[1]*quat[3];
    double yz = quat[2]*quat[3];

    double rr = xx + yy + zz;
    // normalization factor, just in case quaternion was not normalized
    double f = double(1)/double(sqrt(ww + rr));
    double s = (ww - rr)*f;
    f *= 2;

    mat[0][0] = xx*f + s;
    mat[1][0] = (xy + wz)*f;
    mat[2][0] = (xz - wy)*f;

    mat[0][1] = (xy - wz)*f;
    mat[1][1] = yy*f + s;
    mat[2][1] = (yz + wx)*f;

    mat[0][2] = (xz + wy)*f;
    mat[1][2] = (yz - wx)*f;
    mat[2][2] = zz*f + s;
}

inline void quaternionToTransposeMatrix4x4(const double quat[4], vtkMatrix4x4& mat)
{
    double ww = quat[0]*quat[0];
    double wx = quat[0]*quat[1];
    double wy = quat[0]*quat[2];
    double wz = quat[0]*quat[3];

    double xx = quat[1]*quat[1];
    double yy = quat[2]*quat[2];
    double zz = quat[3]*quat[3];

    double xy = quat[1]*quat[2];
    double xz = quat[1]*quat[3];
    double yz = quat[2]*quat[3];

    double rr = xx + yy + zz;
    // normalization factor, just in case quaternion was not normalized
    double f = double(1)/double(sqrt(ww + rr));
    double s = (ww - rr)*f;
    f *= 2;

    mat[0][0] = xx*f + s;
    mat[0][1] = (xy + wz)*f;
    mat[0][2] = (xz - wy)*f;

    mat[1][0] = (xy - wz)*f;
    mat[1][1] = yy*f + s;
    mat[1][2] = (yz + wx)*f;

    mat[2][0] = (xz + wy)*f;
    mat[2][1] = (yz - wx)*f;
    mat[2][2] = zz*f + s;
}

inline double *quatMult(const double q1[4], const double q2[4], double result[4])
{
    double q1w = q1[0];
    double q1x = q1[1];
    double q1y = q1[2];
    double q1z = q1[3];
    double q2w = q2[0];
    double q2x = q2[1];
    double q2y = q2[2];
    double q2z = q2[3];
    result[0] = (q1w*q2w) - (q1x*q2x) - (q1y*q2y) - (q1z*q2z);
    result[1] = (q1w*q2x) + (q1x*q2w) + (q1y*q2z) - (q1z*q2y);
    result[2] = (q1w*q2y) + (q1y*q2w) + (q1z*q2x) - (q1x*q2z);
    result[3] = (q1w*q2z) + (q1z*q2w) + (q1x*q2y) - (q1y*q2x);
    return result;
}

inline double *quatConjugate(const double quat[4], double result[4])
{
    result[1] = -quat[1];
    result[2] = -quat[2];
    result[3] = -quat[3];
    return result;
}

inline double *quatReciprocal(const double quat[4], double result[4])
{
    double denom = 
        quat[0]*quat[0] + 
        quat[1]*quat[1] + 
        quat[2]*quat[2] + 
        quat[3]*quat[3];
    quatConjugate(quat, result);
    for (int i = 0; i < 4; i++) {
        result[i] /= denom;
    }
    return result;
}

inline double *quatReciprocal(double quat[4])
{
    return quatReciprocal(quat, quat);
}

inline void copyQuat(const double quat[4], double result[4])
{
    memcpy(result, quat, sizeof(double)*4);
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
        setViewAngle(_windowHeight);
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
void Renderer::updateRanges()
{
    collectDataRanges();

    for (Contour2DHashmap::iterator itr = _contour2Ds.begin();
         itr != _contour2Ds.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (Contour3DHashmap::iterator itr = _contour3Ds.begin();
         itr != _contour3Ds.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (GlyphsHashmap::iterator itr = _glyphs.begin();
         itr != _glyphs.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (HeightMapHashmap::iterator itr = _heightMaps.begin();
         itr != _heightMaps.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (LICHashmap::iterator itr = _lics.begin();
         itr != _lics.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (MoleculeHashmap::iterator itr = _molecules.begin();
         itr != _molecules.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
         itr != _pseudoColors.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (StreamlinesHashmap::iterator itr = _streamlines.begin();
         itr != _streamlines.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
    for (VolumeHashmap::iterator itr = _volumes.begin();
         itr != _volumes.end(); ++itr) {
        itr->second->updateRanges(_useCumulativeRange, 
                                  _cumulativeScalarRange,
                                  _cumulativeVectorMagnitudeRange,
                                  _cumulativeVectorComponentRange);
    }
}

void Renderer::collectDataRanges()
{
    collectScalarRanges(_cumulativeScalarRange,
                        _cumulativeRangeOnlyVisible);
    collectVectorMagnitudeRanges(_cumulativeVectorMagnitudeRange,
                                 _cumulativeRangeOnlyVisible);
    for (int i = 0; i < 3; i++) {
        collectVectorComponentRanges(_cumulativeVectorComponentRange[i], i,
                                     _cumulativeRangeOnlyVisible);
    }

    TRACE("Cumulative scalar range: %g, %g",
          _cumulativeScalarRange[0],
          _cumulativeScalarRange[1]);
    TRACE("Cumulative vmag range: %g, %g",
          _cumulativeVectorMagnitudeRange[0],
          _cumulativeVectorMagnitudeRange[1]);
    for (int i = 0; i < 3; i++) {
        TRACE("Cumulative v[%d] range: %g, %g", i,
              _cumulativeVectorComponentRange[i][0],
              _cumulativeVectorComponentRange[i][1]);
    }
}

/**
 * \brief Collect cumulative data range of all DataSets
 *
 * \param[in,out] range Data range of all DataSets
 * \param[in] onlyVisible Only collect range of visible DataSets
 */
void Renderer::collectScalarRanges(double *range, bool onlyVisible)
{
    range[0] = DBL_MAX;
    range[1] = -DBL_MAX;

    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility()) {
            double r[2];
            itr->second->getScalarRange(r);
            range[0] = min2(range[0], r[0]);
            range[1] = max2(range[1], r[1]);
        }
    }
    if (range[0] == DBL_MAX)
        range[0] = 0;
    if (range[1] == -DBL_MAX)
        range[1] = 1;
}

/**
 * \brief Collect cumulative data range of all DataSets
 *
 * \param[in,out] range Data range of all DataSets
 * \param[in] onlyVisible Only collect range of visible DataSets
 */
void Renderer::collectVectorMagnitudeRanges(double *range, bool onlyVisible)
{
    range[0] = DBL_MAX;
    range[1] = -DBL_MAX;

    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility()) {
            double r[2];
            itr->second->getVectorRange(r);
            range[0] = min2(range[0], r[0]);
            range[1] = max2(range[1], r[1]);
        }
    }
    if (range[0] == DBL_MAX)
        range[0] = 0;
    if (range[1] == -DBL_MAX)
        range[1] = 1;
}

/**
 * \brief Collect cumulative data range of all DataSets
 *
 * \param[in,out] range Data range of all DataSets
 * \param[in] onlyVisible Only collect range of visible DataSets
 */
void Renderer::collectVectorComponentRanges(double *range, int component, bool onlyVisible)
{
    range[0] = DBL_MAX;
    range[1] = -DBL_MAX;

    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility()) {
            double r[2];
            itr->second->getVectorRange(r, component);
            range[0] = min2(range[0], r[0]);
            range[1] = max2(range[1], r[1]);
        }
    }
    if (range[0] == DBL_MAX)
        range[0] = 0;
    if (range[1] == -DBL_MAX)
        range[1] = 1;
 }

/**
 * \brief Determines if AABB lies in a principal axis plane
 * and if so, returns the plane normal
 */
bool Renderer::is2D(const double bounds[6],
                    Renderer::PrincipalPlane *plane,
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

    if (id.compare("all") == 0 || getContour2D(id) != NULL)
        setContour2DOpacity(id, opacity);
    if (id.compare("all") == 0 || getContour3D(id) != NULL)
        setContour3DOpacity(id, opacity);
    if (id.compare("all") == 0 || getGlyphs(id) != NULL)
        setGlyphsOpacity(id, opacity);
    if (id.compare("all") == 0 || getHeightMap(id) != NULL)
        setHeightMapOpacity(id, opacity);
    if (id.compare("all") == 0 || getLIC(id) != NULL)
        setLICOpacity(id, opacity);
    if (id.compare("all") == 0 || getMolecule(id) != NULL)
        setMoleculeOpacity(id, opacity);
    if (id.compare("all") == 0 || getPolyData(id) != NULL)
        setPolyDataOpacity(id, opacity);
    if (id.compare("all") == 0 || getPseudoColor(id) != NULL)
        setPseudoColorOpacity(id, opacity);
    if (id.compare("all") == 0 || getStreamlines(id) != NULL)
        setStreamlinesOpacity(id, opacity);
    if (id.compare("all") == 0 || getVolume(id) != NULL)
        setVolumeOpacity(id, opacity);

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

    if (id.compare("all") == 0 || getContour2D(id) != NULL)
        setContour2DVisibility(id, state);
    if (id.compare("all") == 0 || getContour3D(id) != NULL)
        setContour3DVisibility(id, state);
    if (id.compare("all") == 0 || getGlyphs(id) != NULL)
        setGlyphsVisibility(id, state);
    if (id.compare("all") == 0 || getHeightMap(id) != NULL)
        setHeightMapVisibility(id, state);
    if (id.compare("all") == 0 || getLIC(id) != NULL)
        setLICVisibility(id, state);
    if (id.compare("all") == 0 || getMolecule(id) != NULL)
        setMoleculeVisibility(id, state);
    if (id.compare("all") == 0 || getPolyData(id) != NULL)
        setPolyDataVisibility(id, state);
    if (id.compare("all") == 0 || getPseudoColor(id) != NULL)
        setPseudoColorVisibility(id, state);
    if (id.compare("all") == 0 || getStreamlines(id) != NULL)
        setStreamlinesVisibility(id, state);
    if (id.compare("all") == 0 || getVolume(id) != NULL)
        setVolumeVisibility(id, state);

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

        if (state) {
            _renderer->AddViewProp(itr->second->getProp());
        }
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
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
            if (ratio > 0.0) {
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
            if (ratio > 0.0) {
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
            if (ratio > 0.0) {
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
