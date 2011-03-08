/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cfloat>

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
#include "Trace.h"

using namespace Rappture::VtkVis;

Renderer::Renderer() :
    _needsRedraw(true),
    _windowWidth(320),
    _windowHeight(320)
{
    _bgColor[0] = 0;
    _bgColor[1] = 0;
    _bgColor[2] = 0;
    // clipping planes to prevent overdrawing axes
    _clippingPlanes = vtkSmartPointer<vtkPlaneCollection>::New();
    // bottom
    vtkPlane *plane0 = vtkPlane::New();
    plane0->SetNormal(0, 1, 0);
    plane0->SetOrigin(0, 0, 0);
    _clippingPlanes->AddItem(plane0);
    // left
    vtkPlane *plane1 = vtkPlane::New();
    plane1->SetNormal(1, 0, 0);
    plane1->SetOrigin(0, 0, 0);
    _clippingPlanes->AddItem(plane1);
   // top
    vtkPlane *plane2 = vtkPlane::New();
    plane2->SetNormal(0, -1, 0);
    plane2->SetOrigin(0, 1, 0);
    _clippingPlanes->AddItem(plane2);
    // right
    vtkPlane *plane3 = vtkPlane::New();
    plane3->SetNormal(-1, 0, 0);
    plane3->SetOrigin(1, 0, 0);
    _clippingPlanes->AddItem(plane3);
    _renderer = vtkSmartPointer<vtkRenderer>::New();
    _renderer->LightFollowCameraOn();
    storeCameraOrientation();
    _cameraMode = PERSPECTIVE;
    initAxes();
    initCamera();
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
    for (DataSetHashmap::iterator itr = _dataSets.begin();
             itr != _dataSets.end(); ++itr) {
        delete itr->second;
    }
    _dataSets.clear();
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
}

/**
 * \brief Add a DataSet to this Renderer
 *
 * This just adds the DataSet to the Renderer's list of data sets.
 * In order to render the data, a PseudoColor or Contour2D must
 * be added to the Renderer.
 */
void Renderer::addDataSet(DataSetId id)
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
void Renderer::deletePseudoColor(DataSetId id)
{
    PseudoColorHashmap::iterator itr = _pseudoColors.find(id);
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting PseudoColors for %s", id.c_str());

    PseudoColor *ps = itr->second;
    if (ps->getActor())
        _renderer->RemoveActor(ps->getActor());
    delete ps;

    _pseudoColors.erase(itr);
    _needsRedraw = true;
}

/**
 * \brief Remove the Contour2D isolines for the specified DataSet
 *
 * The underlying Contour2D is deleted, freeing its memory
 */
void Renderer::deleteContour2D(DataSetId id)
{
    Contour2DHashmap::iterator itr = _contours.find(id);
    if (itr == _contours.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Contour2Ds for %s", id.c_str());

    Contour2D *contour = itr->second;
    if (contour->getActor())
        _renderer->RemoveActor(contour->getActor());
    delete contour;

    _contours.erase(itr);
    _needsRedraw = true;
}

/**
 * \brief Remove the PolyData mesh for the specified DataSet
 *
 * The underlying PolyData is deleted, freeing its memory
 */
void Renderer::deletePolyData(DataSetId id)
{
    PolyDataHashmap::iterator itr = _polyDatas.find(id);
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting PolyDatas for %s", id.c_str());

    PolyData *polyData = itr->second;
    if (polyData->getActor())
        _renderer->RemoveActor(polyData->getActor());
    delete polyData;

    _polyDatas.erase(itr);
    _needsRedraw = true;
}

/**
 * \brief Remove the specified DataSet and associated rendering objects
 *
 * The underlying DataSet and any associated Contour2D and PseudoColor
 * objects are deleted, freeing the memory used.
 */
void Renderer::deleteDataSet(DataSetId id)
{
    DataSetHashmap::iterator itr = _dataSets.find(id);
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return;
    } else {
        TRACE("Deleting dataset %s", id.c_str());

        deletePseudoColor(id);
        deleteContour2D(id);
        deletePolyData(id);
        
        TRACE("After deleting graphics objects");

        // Update cumulative data range
        collectDataRanges(_cumulativeDataRange);

        delete itr->second;
        _dataSets.erase(itr);
         _needsRedraw = true;
    }
}

/**
 * \brief Find the DataSet for the given DataSetId key
 *
 * \return A pointer to the DataSet, or NULL if not found
 */
DataSet *Renderer::getDataSet(DataSetId id)
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
bool Renderer::setDataFile(DataSetId id, const char *filename)
{
    DataSet *ds = getDataSet(id);
    if (ds) {
        bool ret = ds->setDataFile(filename);
        PseudoColor *ps = getPseudoColor(id);
        if (ps) {
            ps->setDataSet(ds);
        }
        _needsRedraw = true;
        return ret;
    } else
        return false;
}

/**
 * \brief (Re-)load the data for the specified DataSet key from a memory buffer
 */
bool Renderer::setData(DataSetId id, char *data, int nbytes)
{
    DataSet *ds = getDataSet(id);
    if (ds) {
        _needsRedraw = true;
        return ds->setData(data, nbytes);
        collectDataRanges(_cumulativeDataRange);
    } else
        return false;
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
    _cubeAxesActor2D->SetFontFactor(2);
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
void Renderer::addColorMap(ColorMapId id, ColorMap *colorMap)
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
ColorMap *Renderer::getColorMap(ColorMapId id)
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
void Renderer::deleteColorMap(ColorMapId id)
{
    ColorMapHashmap::iterator itr = _colorMaps.find(id);
    if (itr == _colorMaps.end())
        return;

    // TODO: Check if color map is used in PseudoColors?
    delete itr->second;
    _colorMaps.erase(itr);
}

/**
 * \brief Render a labelled legend image for the given colormap
 *
 * \return The image is rendered into the supplied array
 */
void Renderer::renderColorMap(ColorMapId id, const char *title,
                              int width, int height,
                              vtkUnsignedCharArray *imgData)
{
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
    _scalarBarActor->SetLookupTable(getColorMap(id)->getLookupTable());
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
}

/**
 * \brief Create a new PseudoColor rendering for the specified DataSet
 */
void Renderer::addPseudoColor(DataSetId id)
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return;

    if (getPseudoColor(id)) {
        WARN("Replacing existing pseudocolor %s", id.c_str());
        deletePseudoColor(id);
    }
    PseudoColor *pc = new PseudoColor();
    _pseudoColors[id] = pc;

    pc->setDataSet(ds);

    _renderer->AddActor(pc->getActor());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Get the PseudoColor associated with the specified DataSet
 */
PseudoColor *Renderer::getPseudoColor(DataSetId id)
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
void Renderer::setPseudoColorColorMap(DataSetId id, ColorMapId colorMapId)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        ColorMap *cmap = getColorMap(colorMapId);
        if (cmap) {
            TRACE("Set color map: %s for dataset %s", colorMapId.c_str(),
                  id.c_str());
            pc->setLookupTable(cmap->getLookupTable());
             _needsRedraw = true;
        } else {
            ERROR("Unknown colormap: %s", colorMapId.c_str());
        }
    } else {
        ERROR("No pseudocolor for dataset %s", id.c_str());
    }
}

/**
 * \brief Get the color map (vtkLookupTable) for the given DataSet
 *
 * \return The associated lookup table or NULL if not found
 */
vtkLookupTable *Renderer::getPseudoColorColorMap(DataSetId id)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc)
        return pc->getLookupTable();
    else
        return NULL;
}

/**
 * \brief Turn on/off rendering of the PseudoColor mapper for the given DataSet
 */
void Renderer::setPseudoColorVisibility(DataSetId id, bool state)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        pc->setVisibility(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setPseudoColorEdgeVisibility(DataSetId id, bool state)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        pc->setEdgeVisibility(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the RGB polygon edge color for the specified DataSet
 */
void Renderer::setPseudoColorEdgeColor(DataSetId id, float color[3])
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        pc->setEdgeColor(color);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the polygon edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setPseudoColorEdgeWidth(DataSetId id, float edgeWidth)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        pc->setEdgeWidth(edgeWidth);
        _needsRedraw = true;
    }
}

/**
 * \brief Turn mesh lighting on/off for the specified DataSet
 */
void Renderer::setPseudoColorLighting(DataSetId id, bool state)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        pc->setLighting(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Create a new Contour2D and associate it with the named DataSet
 */
void Renderer::addContour2D(DataSetId id)
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return;

    if (getContour2D(id)) {
        WARN("Replacing existing contour2d %s", id.c_str());
        deleteContour2D(id);
    }

    Contour2D *contour = new Contour2D();
    _contours[id] = contour;

    contour->setDataSet(ds);

    _renderer->AddActor(contour->getActor());

    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Get the Contour2D associated with a named DataSet
 */
Contour2D *Renderer::getContour2D(DataSetId id)
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
void Renderer::setContours(DataSetId id, int numContours)
{
    Contour2D *contour = getContour2D(id);
    if (contour) {
        contour->setContours(numContours);
        _needsRedraw = true;
    }
}

/**
 * \brief Set a list of isovalues for the given DataSet
 */
void Renderer::setContourList(DataSetId id, const std::vector<double>& contours)
{
    Contour2D *contour = getContour2D(id);
    if (contour) {
        contour->setContourList(contours);
        _needsRedraw = true;
    }
}

/**
 * \brief Turn on/off rendering contour lines for the given DataSet
 */
void Renderer::setContourVisibility(DataSetId id, bool state)
{
    Contour2D *contour = getContour2D(id);
    if (contour) {
        contour->setVisibility(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the RGB isoline color for the specified DataSet
 */
void Renderer::setContourEdgeColor(DataSetId id, float color[3])
{
    Contour2D *contour = getContour2D(id);
    if (contour) {
        contour->setEdgeColor(color);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the isoline width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setContourEdgeWidth(DataSetId id, float edgeWidth)
{
    Contour2D *contour = getContour2D(id);
    if (contour) {
        contour->setEdgeWidth(edgeWidth);
        _needsRedraw = true;
    }
}

/**
 * \brief Turn contour lighting on/off for the specified DataSet
 */
void Renderer::setContourLighting(DataSetId id, bool state)
{
    Contour2D *contour = getContour2D(id);
    if (contour) {
        contour->setLighting(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Create a new PolyData and associate it with the named DataSet
 */
void Renderer::addPolyData(DataSetId id)
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return;

    if (getPolyData(id)) {
        WARN("Replacing existing polydata %s", id.c_str());
        deletePolyData(id);
    }

    PolyData *polyData = new PolyData();
    _polyDatas[id] = polyData;

    polyData->setDataSet(ds);

    _renderer->AddActor(polyData->getActor());

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
    _needsRedraw = true;
}

/**
 * \brief Get the PolyData associated with a named DataSet
 */
PolyData *Renderer::getPolyData(DataSetId id)
{
    PolyDataHashmap::iterator itr = _polyDatas.find(id);

    if (itr == _polyDatas.end()) {
        TRACE("PolyData not found: %s", id.c_str());
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Turn on/off rendering of the PolyData mapper for the given DataSet
 */
void Renderer::setPolyDataVisibility(DataSetId id, bool state)
{
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setVisibility(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the RGB polygon face color for the specified DataSet
 */
void Renderer::setPolyDataColor(DataSetId id, float color[3])
{
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setColor(color);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the visibility of polygon edges for the specified DataSet
 */
void Renderer::setPolyDataEdgeVisibility(DataSetId id, bool state)
{
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setEdgeVisibility(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the RGB polygon edge color for the specified DataSet
 */
void Renderer::setPolyDataEdgeColor(DataSetId id, float color[3])
{
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setEdgeColor(color);
        _needsRedraw = true;
    }
}

/**
 * \brief Set the polygon edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setPolyDataEdgeWidth(DataSetId id, float edgeWidth)
{
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setEdgeWidth(edgeWidth);
        _needsRedraw = true;
    }
}

/**
 * \brief Set wireframe rendering for the specified DataSet
 */
void Renderer::setPolyDataWireframe(DataSetId id, bool state)
{
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setWireframe(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Turn mesh lighting on/off for the specified DataSet
 */
void Renderer::setPolyDataLighting(DataSetId id, bool state)
{
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setLighting(state);
        _needsRedraw = true;
    }
}

/**
 * \brief Resize the render window (image size for renderings)
 */
void Renderer::setWindowSize(int width, int height)
{
    _windowWidth = width;
    _windowHeight = height;
    _renderWindow->SetSize(_windowWidth, _windowHeight);
#ifdef notdef
    setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                        _imgWorldDims[0], _imgWorldDims[1]);
#endif
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
    }
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
    _needsRedraw = true;
}

/**
 * \brief Perform a relative 2D translation of the camera
 *
 * \param[in] x World coordinate horizontal panning
 * \param[in] y World coordinate vertical panning
 */
void Renderer::panCamera(double x, double y)
{
    if (_cameraMode == IMAGE) {
        _imgWorldOrigin[0] += x;
        _imgWorldOrigin[1] += y;
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0], _imgWorldDims[1]);
        _needsRedraw = true;
        return;
    }
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
    trans->Translate(x, y, 0);
    camera->ApplyTransform(trans);
    _renderer->ResetCameraClippingRange();
    storeCameraOrientation();
    _needsRedraw = true;
}

/**
 * \brief Change the FOV of the camera
 *
 * \param[in] z Ratio to change zoom (greater than 1 is zoom in, less than 1 is zoom out)
 */
void Renderer::zoomCamera(double z)
{
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
        _needsRedraw = true;
        return;
    }
    vtkSmartPointer<vtkCamera> camera = _renderer->GetActiveCamera();
    camera->Zoom(z); // Change perspective FOV angle or ortho parallel scale
    //camera->Dolly(z); // Move camera forward/back
    _renderer->ResetCameraClippingRange();
    storeCameraOrientation();
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

    int pxOffsetX = 75;
    int pxOffsetY = 75;
    int outerGutter = 15;

    int imgHeightPx = _windowHeight - pxOffsetY - outerGutter;
    double pxToWorld = height / imgHeightPx;
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

#ifdef DEBUG
    printCameraInfo(camera);
#endif

    _needsRedraw = true;
}

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
 * \brief Collect cumulative data range of all DataSets
 *
 * \param[inout] range Data range of all DataSets
 */
void Renderer::collectDataRanges(double *range)
{
    range[0] = DBL_MAX;
    range[1] = -DBL_MAX;

    for (DataSetHashmap::iterator itr = _dataSets.begin();
         itr != _dataSets.end(); ++itr) {
        double r[2];
        itr->second->getDataRange(r);
        range[0] = min2(range[0], r[0]);
        range[1] = max2(range[1], r[1]);
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

    if (_cameraMode == IMAGE) {
        _renderer->ResetCamera();
        setCameraZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                            _imgWorldDims[0], _imgWorldDims[1]);
        resetAxes();
    } else if (_cameraMode == ORTHO) {
        _renderer->GetActiveCamera()->ParallelProjectionOn();
        resetAxes();
        _renderer->ResetCamera();
    } else if (_cameraMode == PERSPECTIVE) {
        _renderer->GetActiveCamera()->ParallelProjectionOff();
        resetAxes();
        _renderer->ResetCamera();
    }
}

/**
 * \brief Print debugging info about a vtkCamera
 */
void Renderer::printCameraInfo(vtkCamera *camera)
{
    TRACE("Parallel Scale: %g, Cam pos: %g %g %g, Clipping range: %g %g",
          camera->GetParallelScale(),
	  camera->GetPosition()[0],
          camera->GetPosition()[1],
          camera->GetPosition()[2], 
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
void Renderer::setOpacity(DataSetId id, double opacity)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        pc->setOpacity(opacity);
        _needsRedraw = true;
    }
    Contour2D *contour = getContour2D(id);
    if (contour) {
        contour->setOpacity(opacity);
        _needsRedraw = true;
    }
    PolyData *polyData = getPolyData(id);
    if (polyData) {
        polyData->setOpacity(opacity);
        _needsRedraw = true;
    }
}

/**
 * \brief Turn on/off rendering of the specified DataSet's associated graphics objects
 */
void Renderer::setVisibility(DataSetId id, bool state)
{
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
double Renderer::getDataValueAtPixel(DataSetId id, int x, int y)
{
    vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
    coord->SetCoordinateSystemToDisplay();
    coord->SetValue(x, y, 0);
    double *worldCoords = coord->GetComputedWorldValue(_renderer);

#ifdef DEBUG
    TRACE("Pixel coords: %d, %d\nWorld coords: %.12e, %.12e, %12e", x, y,
          worldCoords[0], 
          worldCoords[1], 
          worldCoords[2]);
#endif

    return getDataValue(id, worldCoords[0], worldCoords[1], worldCoords[2]);
}

/**
 * \brief Get nearest data value given world coordinates x,y,z
 *
 * Note: no interpolation is performed on data
 */
double Renderer::getDataValue(DataSetId id, double x, double y, double z)
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return 0;
    vtkDataSet *vtkds = ds->getVtkDataSet(); 
    vtkIdType pt = vtkds->FindPoint(x, y, z);
    return vtkds->GetPointData()->GetScalars()->GetComponent(pt, 0);
}
