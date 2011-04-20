/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cfloat>
#include <cstring>
#include <cassert>

#include <vtkCamera.h>
#include <vtkCoordinate.h>
#include <vtkTransform.h>
#include <vtkCharArray.h>
#include <vtkAxisActor2D.h>
#ifdef USE_CUSTOM_AXES
#include <vtkRpCubeAxesActor2D.h>
#else
#include <vtkCubeAxesActor.h>
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

#include "RpVtkRenderer.h"
#include "ColorMap.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Renderer::Renderer() :
    _needsRedraw(true),
    _windowWidth(320),
    _windowHeight(320),
    _cameraZoomRatio(1),
    _useCumulativeRange(true),
    _cumulativeRangeOnlyVisible(false)
{
    _bgColor[0] = 0;
    _bgColor[1] = 0;
    _bgColor[2] = 0;
    _cameraPan[0] = 0;
    _cameraPan[1] = 0;
    _cumulativeDataRange[0] = 0.0;
    _cumulativeDataRange[1] = 1.0;
    // clipping planes to prevent overdrawing axes
    _clippingPlanes = vtkSmartPointer<vtkPlaneCollection>::New();
    // bottom
    vtkSmartPointer<vtkPlane> plane0 = vtkSmartPointer<vtkPlane>::New();
    plane0->SetNormal(0, 1, 0);
    plane0->SetOrigin(0, 0, 0);
    _clippingPlanes->AddItem(plane0);
    // left
    vtkSmartPointer<vtkPlane> plane1 = vtkSmartPointer<vtkPlane>::New();
    plane1->SetNormal(1, 0, 0);
    plane1->SetOrigin(0, 0, 0);
    _clippingPlanes->AddItem(plane1);
   // top
    vtkSmartPointer<vtkPlane> plane2 = vtkSmartPointer<vtkPlane>::New();
    plane2->SetNormal(0, -1, 0);
    plane2->SetOrigin(0, 1, 0);
    _clippingPlanes->AddItem(plane2);
    // right
    vtkSmartPointer<vtkPlane> plane3 = vtkSmartPointer<vtkPlane>::New();
    plane3->SetNormal(-1, 0, 0);
    plane3->SetOrigin(1, 0, 0);
    _clippingPlanes->AddItem(plane3);
    _renderer = vtkSmartPointer<vtkRenderer>::New();
    _renderer->LightFollowCameraOn();
    _cameraMode = PERSPECTIVE;
    initAxes();
    initCamera();
    storeCameraOrientation();
    _renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    _renderWindow->DoubleBufferOff();
    //_renderWindow->BordersOff();
    _renderWindow->SetSize(_windowWidth, _windowHeight);
    _renderWindow->OffScreenRenderingOn();
    _renderWindow->AddRenderer(_renderer);
    addColorMap("default", ColorMap::createDefault());
}

Renderer::~Renderer()
{
    for (ColorMapHashmap::iterator itr = _colorMaps.begin();
             itr != _colorMaps.end(); ++itr) {
        delete itr->second;
    }
    _colorMaps.clear();
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
             itr != _pseudoColors.end(); ++itr) {
        delete itr->second;
    }
    _pseudoColors.clear();
    for (Contour2DHashmap::iterator itr = _contours.begin();
             itr != _contours.end(); ++itr) {
        delete itr->second;
    }
    _contours.clear();
    for (PolyDataHashmap::iterator itr = _polyDatas.begin();
             itr != _polyDatas.end(); ++itr) {
        delete itr->second;
    }
    _polyDatas.clear();
    for (DataSetHashmap::iterator itr = _dataSets.begin();
             itr != _dataSets.end(); ++itr) {
        delete itr->second;
    }
    _dataSets.clear();
}

/**
 * \brief Add a DataSet to this Renderer
 *
 * This just adds the DataSet to the Renderer's list of data sets.
 * In order to render the data, a PseudoColor or Contour2D must
 * be added to the Renderer.
 */
void Renderer::addDataSet(const DataSetId& id)
{
    if (getDataSet(id) != NULL) {
        WARN("Replacing existing dataset %s", id.c_str());
        deleteDataSet(id);
    }
    _dataSets[id] = new DataSet(id);
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
        if (ps->getActor())
            _renderer->RemoveActor(ps->getActor());
        delete ps;

        _pseudoColors.erase(itr);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
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
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Contour2Ds for %s", id.c_str());

    do {
        Contour2D *contour = itr->second;
        if (contour->getActor())
            _renderer->RemoveActor(contour->getActor());
        delete contour;

        _contours.erase(itr);
    } while (doAll && ++itr != _contours.end());

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
        if (polyData->getActor())
            _renderer->RemoveActor(polyData->getActor());
        delete polyData;

        _polyDatas.erase(itr);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Remove the specified DataSet and associated rendering objects
 *
 * The underlying DataSet and any associated Contour2D and PseudoColor
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

        deletePseudoColor(itr->second->getName());
        deleteContour2D(itr->second->getName());
        deletePolyData(itr->second->getName());
    
        TRACE("After deleting graphics objects");

        delete itr->second;
        _dataSets.erase(itr);
    } while (doAll && ++itr != _dataSets.end());

    // Update cumulative data range
    collectDataRanges(_cumulativeDataRange, _cumulativeRangeOnlyVisible);
    updateRanges(_useCumulativeRange);

    _needsRedraw = true;
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
        TRACE("DataSet not found: %s", id.c_str());
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
        collectDataRanges(_cumulativeDataRange, _cumulativeRangeOnlyVisible);
        updateRanges(_useCumulativeRange);
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
        collectDataRanges(_cumulativeDataRange, _cumulativeRangeOnlyVisible);
        updateRanges(_useCumulativeRange);
        _needsRedraw = true;
        return ret;
    } else
        return false;
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
        collectDataRanges(_cumulativeDataRange, _cumulativeRangeOnlyVisible);
        updateRanges(_useCumulativeRange);
        _needsRedraw = true;
    }
}

void Renderer::resetAxes()
{
    TRACE("Resetting axes");
    if (_cubeAxesActor == NULL ||
        _cubeAxesActor2D == NULL) {
        initAxes();
    }
    if (_cameraMode == IMAGE) {
        if (_renderer->HasViewProp(_cubeAxesActor)) {
            TRACE("Removing 3D axes");
            _renderer->RemoveActor(_cubeAxesActor);
        }
        if (!_renderer->HasViewProp(_cubeAxesActor2D)) {
            TRACE("Adding 2D axes");
            _renderer->AddActor(_cubeAxesActor2D);
        }
    } else {
        if (_renderer->HasViewProp(_cubeAxesActor2D)) {
            TRACE("Removing 2D axes");
            _renderer->RemoveActor(_cubeAxesActor2D);
        }
        if (!_renderer->HasViewProp(_cubeAxesActor)) {
            TRACE("Adding 3D axes");
            _renderer->AddActor(_cubeAxesActor);
        }
        double bounds[6];
        collectBounds(bounds, true);
        _cubeAxesActor->SetBounds(bounds);
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
            _renderer->AddActor(_cubeAxesActor2D);
    } else {
        if (!_renderer->HasViewProp(_cubeAxesActor))
            _renderer->AddActor(_cubeAxesActor);
    }
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
#else
        _cubeAxesActor2D->GetAxisTitleTextProperty()->SetColor(color);
        _cubeAxesActor2D->GetAxisLabelTextProperty()->SetColor(color);
#endif
        _needsRedraw = true;
    }
}

/**
 * \brief Turn on/off rendering of all axes gridlines
 */
void Renderer::setAxesGridVisibility(bool state)
{
    if (_cubeAxesActor != NULL) {
        _cubeAxesActor->SetDrawXGridlines((state ? 1 : 0));
        _cubeAxesActor->SetDrawYGridlines((state ? 1 : 0));
        _cubeAxesActor->SetDrawZGridlines((state ? 1 : 0));
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
 * \brief Turn on/off rendering of all axes
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
    setAxisVisibility(X_AXIS, state);
    setAxisVisibility(Y_AXIS, state);
    setAxisVisibility(Z_AXIS, state);
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
        }
        _needsRedraw = true;
    }
#endif
}

/**
 * \brief Add a color map for use in the Renderer
 */
void Renderer::addColorMap(const ColorMapId& id, ColorMap *colorMap)
{
    if (colorMap != NULL) {
        colorMap->build();
        if (getColorMap(id) != NULL) {
            WARN("Replacing existing colormap %s", id.c_str());
            deleteColorMap(id);
        }
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
        TRACE("Deleting ColorMap %s", itr->second->getName().c_str());

        // TODO: Check if color map is used in PseudoColors?
        delete itr->second;
        _colorMaps.erase(itr);
    } while (doAll && ++itr != _colorMaps.end());
}

/**
 * \brief Render a labelled legend image for the given colormap
 *
 * \return The image is rendered into the supplied array, false is 
 * returned if the color map is not found
 */
bool Renderer::renderColorMap(const ColorMapId& id, 
                              const DataSetId& dataSetID,
                              const char *title,
                              int width, int height,
                              vtkUnsignedCharArray *imgData)
{
    ColorMap *colorMap = getColorMap(id);
    if (colorMap == NULL)
        return false;

    if (_legendRenderWindow == NULL) {
        _legendRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
        _legendRenderWindow->DoubleBufferOff();
        _legendRenderWindow->OffScreenRenderingOn();
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
        _legendRenderer->AddActor(_scalarBarActor);
    }

    vtkSmartPointer<vtkLookupTable> lut = colorMap->getLookupTable();
    if (dataSetID.compare("all") == 0) {
        lut->SetRange(_cumulativeDataRange);
    } else {
        DataSet *dataSet = getDataSet(dataSetID);
        if (dataSet == NULL) {
            lut->SetRange(_cumulativeDataRange);
        } else {
            double range[2];
            dataSet->getDataRange(range);
            lut->SetRange(range);
        }
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
    _scalarBarActor->SetTitle(title);
    _scalarBarActor->GetTitleTextProperty()->ItalicOff();
    _scalarBarActor->GetLabelTextProperty()->BoldOff();
    _scalarBarActor->GetLabelTextProperty()->ItalicOff();
    _scalarBarActor->GetLabelTextProperty()->ShadowOff();

    _legendRenderWindow->Render();

    _legendRenderWindow->GetPixelData(0, 0, width-1, height-1, 1, imgData);
    return true;
}

/**
 * \brief Create a new PseudoColor rendering for the specified DataSet
 */
void Renderer::addPseudoColor(const DataSetId& id)
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
        return;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getPseudoColor(dsID)) {
            WARN("Replacing existing pseudocolor %s", dsID.c_str());
            deletePseudoColor(dsID);
        }
        PseudoColor *pc = new PseudoColor();
        _pseudoColors[dsID] = pc;

        pc->setDataSet(ds);

        // Use the default color map
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        ColorMap *cmap = getColorMap("default");
        lut->DeepCopy(cmap->getLookupTable());
        if (_useCumulativeRange) {
            lut->SetRange(_cumulativeDataRange);
        } else {
            double range[2];
            ds->getDataRange(range);
            lut->SetRange(range);
        }

        pc->setLookupTable(lut);

        _renderer->AddActor(pc->getActor());
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Get the PseudoColor associated with the specified DataSet
 */
PseudoColor *Renderer::getPseudoColor(const DataSetId& id)
{
    PseudoColorHashmap::iterator itr = _pseudoColors.find(id);

    if (itr == _pseudoColors.end()) {
        TRACE("PseudoColor not found: %s", id.c_str());
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Associate an existing named color map with a DataSet
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
        TRACE("Set color map: %s for dataset %s", colorMapId.c_str(),
              itr->second->getDataSet()->getName().c_str());

        // Make a copy of the generic colormap lookup table, so 
        // data range can be set in the copy table to match the 
        // dataset being plotted
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        lut->DeepCopy(cmap->getLookupTable());

        if (_useCumulativeRange) {
            lut->SetRange(_cumulativeDataRange);
        } else {
            if (itr->second->getDataSet() != NULL) {
                double range[2];
                itr->second->getDataSet()->getDataRange(range);
                lut->SetRange(range);
            }
        }

        itr->second->setLookupTable(lut);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Get the color map (vtkLookupTable) for the given DataSet
 *
 * \return The associated lookup table or NULL if not found
 */
vtkLookupTable *Renderer::getPseudoColorColorMap(const DataSetId& id)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc)
        return pc->getLookupTable();
    else
        return NULL;
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
 * \brief Create a new Contour2D and associate it with the named DataSet
 */
void Renderer::addContour2D(const DataSetId& id)
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
        return;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getContour2D(dsID)) {
            WARN("Replacing existing contour2d %s", dsID.c_str());
            deleteContour2D(dsID);
        }

        Contour2D *contour = new Contour2D();
        _contours[dsID] = contour;

        contour->setDataSet(ds);

        _renderer->AddActor(contour->getActor());
    } while (doAll && ++itr != _dataSets.end());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Get the Contour2D associated with a named DataSet
 */
Contour2D *Renderer::getContour2D(const DataSetId& id)
{
    Contour2DHashmap::iterator itr = _contours.find(id);

    if (itr == _contours.end()) {
        TRACE("Contour2D not found: %s", id.c_str());
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Set the number of equally spaced contour isolines for the given DataSet
 */
void Renderer::setContours(const DataSetId& id, int numContours)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        if (_useCumulativeRange) {
            itr->second->setContours(numContours, _cumulativeDataRange);
        } else {
            itr->second->setContours(numContours);
        }
    } while (doAll && ++itr != _contours.end());

    _needsRedraw = true;
}

/**
 * \brief Set a list of isovalues for the given DataSet
 */
void Renderer::setContourList(const DataSetId& id, const std::vector<double>& contours)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourList(contours);
    } while (doAll && ++itr != _contours.end());

     _needsRedraw = true;
}

/**
 * \brief Set opacity of contour lines for the given DataSet
 */
void Renderer::setContourOpacity(const DataSetId& id, double opacity)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != _contours.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering contour lines for the given DataSet
 */
void Renderer::setContourVisibility(const DataSetId& id, bool state)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != _contours.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB isoline color for the specified DataSet
 */
void Renderer::setContourEdgeColor(const DataSetId& id, float color[3])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != _contours.end());

    _needsRedraw = true;
}

/**
 * \brief Set the isoline width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setContourEdgeWidth(const DataSetId& id, float edgeWidth)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _contours.end());

    _needsRedraw = true;
}

/**
 * \brief Turn contour lighting on/off for the specified DataSet
 */
void Renderer::setContourLighting(const DataSetId& id, bool state)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contours.begin();
        doAll = true;
    } else {
        itr = _contours.find(id);
    }
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != _contours.end());
    _needsRedraw = true;
}

/**
 * \brief Create a new PolyData and associate it with the named DataSet
 */
void Renderer::addPolyData(const DataSetId& id)
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
        return;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        if (getPolyData(dsID)) {
            WARN("Replacing existing polydata %s", dsID.c_str());
            deletePolyData(dsID);
        }

        PolyData *polyData = new PolyData();
        _polyDatas[dsID] = polyData;

        polyData->setDataSet(ds);

        _renderer->AddActor(polyData->getActor());
    } while (doAll && ++itr != _dataSets.end());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Get the PolyData associated with a named DataSet
 */
PolyData *Renderer::getPolyData(const DataSetId& id)
{
    PolyDataHashmap::iterator itr = _polyDatas.find(id);

    if (itr == _polyDatas.end()) {
        TRACE("PolyData not found: %s", id.c_str());
        return NULL;
    } else
        return itr->second;
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
 * \brief Resize the render window (image size for renderings)
 */
void Renderer::setWindowSize(int width, int height)
{
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
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    switch (mode) {
    case ORTHO: {
        TRACE("Set camera to Ortho mode");
        camera->ParallelProjectionOn();
        if (origMode == IMAGE) {
            resetCamera(false);
        }
        break;
    }
    case PERSPECTIVE: {
        TRACE("Set camera to Perspective mode");
        camera->ParallelProjectionOff();
        if (origMode == IMAGE) {
            resetCamera(false);
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
    resetAxes();
    _needsRedraw = true;
}

/**
 * \brief Get the current camera mode
 */
Renderer::CameraMode Renderer::getCameraMode() const
{
    return _cameraMode;
}

void Renderer::setSceneOrientation(double quat[4])
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkPerspectiveTransform>::New();
    double mat3[3][3];
    vtkMath::QuaternionToMatrix3x3(quat, mat3);
    vtkSmartPointer<vtkMatrix4x4> mat4 = vtkSmartPointer<vtkMatrix4x4>::New();
    for (int r = 0; r < 3; r++) {
        memcpy(mat4[r], mat3[r], sizeof(double)*3);
    }
    trans->Translate(+_cameraFocalPoint[0], +_cameraFocalPoint[1], +_cameraFocalPoint[2]);
    trans->Concatenate(mat4);
    trans->Translate(-_cameraFocalPoint[0], -_cameraFocalPoint[1], -_cameraFocalPoint[2]);
    camera->SetPosition(0, 0, 1);
    camera->SetFocalPoint(0, 0, 0);
    camera->SetViewUp(0, 1, 0);
    camera->ApplyTransform(trans);
    storeCameraOrientation();
    if (_cameraPan[0] != 0.0 || _cameraPan[1] != 0.0) {
        panCamera(_cameraPan[0], _cameraPan[1], true);
    }
    _needsRedraw = true;
}

void Renderer::setCameraOrientationAndPosition(double position[3],
                                               double focalPoint[3],
                                               double viewUp[3])
{
    memcpy(_cameraPos, position, sizeof(double)*3);
    memcpy(_cameraFocalPoint, focalPoint, sizeof(double)*3);
    memcpy(_cameraUp, viewUp, sizeof(double)*3);
    // Apply the new parameters to the VTK camera
    restoreCameraOrientation();
    _needsRedraw = true;
}

void Renderer::getCameraOrientationAndPosition(double position[3],
                                               double focalPoint[3],
                                               double viewUp[3])
{
    memcpy(position, _cameraPos, sizeof(double)*3);
    memcpy(focalPoint, _cameraFocalPoint, sizeof(double)*3);
    memcpy(viewUp, _cameraUp, sizeof(double)*3);
}

void Renderer::storeCameraOrientation()
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->GetPosition(_cameraPos);
    camera->GetFocalPoint(_cameraFocalPoint);
    camera->GetViewUp(_cameraUp);
}

void Renderer::restoreCameraOrientation()
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->SetPosition(_cameraPos);
    camera->SetFocalPoint(_cameraFocalPoint);
    camera->SetViewUp(_cameraUp);
}

/**
 * \brief Reset pan, zoom, clipping planes and optionally rotation
 *
 * \param[in] resetOrientation Reset the camera rotation/orientation also
 */
void Renderer::resetCamera(bool resetOrientation)
{
    if (_cameraMode == IMAGE) {
        initCamera();
    } else {
        if (resetOrientation) {
            vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
            camera->SetPosition(0, 0, 1);
            camera->SetFocalPoint(0, 0, 0);
            camera->SetViewUp(0, 1, 0);
            storeCameraOrientation();
        } else {
            restoreCameraOrientation();
        }
        _renderer->ResetCamera();
        _renderer->ResetCameraClippingRange();
        computeScreenWorldCoords();
    }
    _cameraZoomRatio = 1;
    _cameraPan[0] = 0;
    _cameraPan[1] = 0;

    _needsRedraw = true;
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
    storeCameraOrientation();
    computeScreenWorldCoords();
    _needsRedraw = true;
}

/**
 * \brief Perform a 2D translation of the camera
 *
 * x,y pan amount are specified as signed absolute pan amount in viewport
 * units -- i.e. 0 is no pan, .5 is half the viewport, 2 is twice the viewport,
 * etc.
 *
 * \param[in] x Viewport coordinate horizontal panning
 * \param[in] y Viewport coordinate vertical panning (with origin at top)
 * \param[in] absolute Control if pan amount is relative to current or absolute
 */
void Renderer::panCamera(double x, double y, bool absolute)
{
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
        storeCameraOrientation();
        computeScreenWorldCoords();
    }
    _needsRedraw = true;
}

/**
 * \brief Change the FOV of the camera
 *
 * \param[in] z Ratio to change zoom (greater than 1 is zoom in, less than 1 is zoom out)
 * \param[in] absolute Control if zoom factor is relative to current setting or absolute
 */
void Renderer::zoomCamera(double z, bool absolute)
{
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
        vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
        camera->Zoom(z); // Change perspective FOV angle or ortho parallel scale
        //camera->Dolly(z); // Move camera forward/back
        _renderer->ResetCameraClippingRange();
        storeCameraOrientation();
    }
    _needsRedraw = true;
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

    int pxOffsetX = 85;
    int pxOffsetY = 75;
    int outerGutter = 15;

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
    camera->SetPosition(camPos[0], camPos[1], 1);
    camera->SetFocalPoint(camPos[0], camPos[1], 0);
    camera->SetViewUp(0, 1, 0);
    camera->SetClippingRange(1, 2);
    // Half of world coordinate height of viewport (Documentation is wrong)
    camera->SetParallelScale(_windowHeight * pxToWorld / 2.0);

    // bottom
    _clippingPlanes->GetItem(0)->SetOrigin(0, _imgWorldOrigin[1], 0);
    // left
    _clippingPlanes->GetItem(1)->SetOrigin(_imgWorldOrigin[0], 0, 0);
    // top
    _clippingPlanes->GetItem(2)->SetOrigin(0, _imgWorldOrigin[1] + _imgWorldDims[1], 0);
    // right
    _clippingPlanes->GetItem(3)->SetOrigin(_imgWorldOrigin[0] + _imgWorldDims[0], 0, 0);

    _cubeAxesActor2D->SetBounds(_imgWorldOrigin[0], _imgWorldOrigin[0] + _imgWorldDims[0],
                                _imgWorldOrigin[1], _imgWorldOrigin[1] + _imgWorldDims[1], 0, 0);

    // Compute screen world coordinates
    computeScreenWorldCoords();

#ifdef DEBUG
    printCameraInfo(camera);
#endif

    _needsRedraw = true;
}

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

void Renderer::computeWorldToDisplay(double x, double y, double z, double displayPt[3])
{
    _renderer->SetWorldPoint(x, y, z, 1.0);
    _renderer->WorldToDisplay();
    _renderer->GetDisplayPoint(displayPt);
}

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

    _screenWorldCoords[0] = x0;
    _screenWorldCoords[1] = y0;
    _screenWorldCoords[2] = x1 - x0;
    _screenWorldCoords[3] = y1 - y0;
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

    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
             itr != _pseudoColors.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility())
            mergeBounds(bounds, bounds, itr->second->getActor()->GetBounds());
    }
    for (Contour2DHashmap::iterator itr = _contours.begin();
             itr != _contours.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility())
            mergeBounds(bounds, bounds, itr->second->getActor()->GetBounds());
    }
    for (PolyDataHashmap::iterator itr = _polyDatas.begin();
             itr != _polyDatas.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility())
            mergeBounds(bounds, bounds, itr->second->getActor()->GetBounds());
    }
    for (int i = 0; i < 6; i++) {
        if (i % 2 == 0) {
            if (bounds[i] == DBL_MAX)
                bounds[i] = 0;
        } else {
            if (bounds[i] == -DBL_MAX)
                bounds[i] = 1;
        }
    }
}

/**
 * \brief Update data ranges for color-mapping and contours
 *
 * \param[in] useCumulative Use cumulative range of all DataSets
 */
void Renderer::updateRanges(bool useCumulative)
{
    for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
         itr != _pseudoColors.end(); ++itr) {
        vtkLookupTable *lut = itr->second->getLookupTable();
        if (lut) {
            if (useCumulative) {
                lut->SetRange(_cumulativeDataRange);
            } else {
                double range[2];
                if (itr->second->getDataSet()) {
                    itr->second->getDataSet()->getDataRange(range);
                    lut->SetRange(range);
                }
            }
        }
    }
    for (Contour2DHashmap::iterator itr = _contours.begin();
         itr != _contours.end(); ++itr) {
        // Only need to update range if using evenly spaced contours
        if (itr->second->getContourList().empty()) {
            if (useCumulative) {
                itr->second->setContours(itr->second->getNumContours(), _cumulativeDataRange);
            } else {
                itr->second->setContours(itr->second->getNumContours());
            }
        }
    }
}

/**
 * \brief Collect cumulative data range of all DataSets
 *
 * \param[inout] range Data range of all DataSets
 * \param[in] onlyVisible Only collect range of visible DataSets
 */
void Renderer::collectDataRanges(double *range, bool onlyVisible)
{
    range[0] = DBL_MAX;
    range[1] = -DBL_MAX;

    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        if (!onlyVisible || itr->second->getVisibility()) {
            double r[2];
            itr->second->getDataRange(r);
            range[0] = min2(range[0], r[0]);
            range[1] = max2(range[1], r[1]);
        }
    }
    if (range[0] == DBL_MAX)
        range[0] = 0;
    if (range[1] == -DBL_MAX)
        range[1] = 1;
}

#ifdef notdef
void Renderer::setPerspectiveCameraByBounds(double bounds[6])
{
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->ParallelProjectionOff();
    camera->Reset();
}
#endif

/**
 * \brief Initialize the camera zoom region to include the bounding volume given
 */
void Renderer::initCamera()
{
    double bounds[6];
    collectBounds(bounds, true);
    _imgWorldOrigin[0] = bounds[0];
    _imgWorldOrigin[1] = bounds[2];
    _imgWorldDims[0] = bounds[1] - bounds[0];
    _imgWorldDims[1] = bounds[3] - bounds[2];
    _cameraPan[0] = 0;
    _cameraPan[1] = 0;
    _cameraZoomRatio = 1;

    if (_cameraMode == IMAGE) {
        _renderer->ResetCamera();
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0], _imgWorldDims[1]);
        resetAxes();
    } else if (_cameraMode == ORTHO) {
        _renderer->GetActiveCamera()->ParallelProjectionOn();
        resetAxes();
        _renderer->ResetCamera();
        computeScreenWorldCoords();
    } else if (_cameraMode == PERSPECTIVE) {
        _renderer->GetActiveCamera()->ParallelProjectionOff();
        resetAxes();
        _renderer->ResetCamera();
        computeScreenWorldCoords();
    }
}

/**
 * \brief Print debugging info about a vtkCamera
 */
void Renderer::printCameraInfo(vtkCamera *camera)
{
    TRACE("Parallel Scale: %g, Cam pos: %g %g %g, focal pt: %g %g %g, view up: %g %g %g, Clipping range: %g %g",
          camera->GetParallelScale(),
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
void Renderer::setOpacity(const DataSetId& id, double opacity)
{
    setPseudoColorOpacity(id, opacity);
    setContourOpacity(id, opacity);
    setPolyDataOpacity(id, opacity);
}

/**
 * \brief Turn on/off rendering of the specified DataSet's associated graphics objects
 */
void Renderer::setVisibility(const DataSetId& id, bool state)
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

    setPseudoColorVisibility(id, state);
    setContourVisibility(id, state);
    setPolyDataVisibility(id, state);
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
        if (_cameraMode == IMAGE) {
            for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
                 itr != _pseudoColors.end(); ++itr) {
                itr->second->setClippingPlanes(_clippingPlanes);
            }
            for (Contour2DHashmap::iterator itr = _contours.begin();
                 itr != _contours.end(); ++itr) {
                itr->second->setClippingPlanes(_clippingPlanes);
            }
            for (PolyDataHashmap::iterator itr = _polyDatas.begin();
                 itr != _polyDatas.end(); ++itr) {
                itr->second->setClippingPlanes(_clippingPlanes);
            }
        } else {
            for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
                 itr != _pseudoColors.end(); ++itr) {
                itr->second->setClippingPlanes(NULL);
            }
            for (Contour2DHashmap::iterator itr = _contours.begin();
                 itr != _contours.end(); ++itr) {
                itr->second->setClippingPlanes(NULL);
            }
            for (PolyDataHashmap::iterator itr = _polyDatas.begin();
                 itr != _polyDatas.end(); ++itr) {
                itr->second->setClippingPlanes(NULL);
            }
        }
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
    _renderWindow->GetPixelData(0, 0, _windowWidth-1, _windowHeight-1, 1, imgData);
    TRACE("Image data size: %d", imgData->GetSize());
}

/**
 * \brief Get nearest data value given display coordinates x,y
 *
 * Note: no interpolation is performed on data
 */
double Renderer::getDataValueAtPixel(const DataSetId& id, int x, int y)
{
    vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
    coord->SetCoordinateSystemToDisplay();
    coord->SetValue(x, y, 0);
    double *worldCoords = coord->GetComputedWorldValue(_renderer);

    TRACE("Pixel coords: %d, %d\nWorld coords: %.12e, %.12e, %12e", x, y,
          worldCoords[0], 
          worldCoords[1], 
          worldCoords[2]);

    return getDataValue(id, worldCoords[0], worldCoords[1], worldCoords[2]);
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 */
double Renderer::getDataValue(const DataSetId& id, double x, double y, double z)
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return 0;
    vtkDataSet *vtkds = ds->getVtkDataSet(); 
    vtkIdType pt = vtkds->FindPoint(x, y, z);
    return vtkds->GetPointData()->GetScalars()->GetComponent(pt, 0);
}
