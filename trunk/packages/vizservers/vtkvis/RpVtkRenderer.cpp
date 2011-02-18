/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkCamera.h>
#include <vtkCoordinate.h>
#include <vtkCharArray.h>
#include <vtkAxisActor2D.h>
#ifdef USE_CUSTOM_AXES
#include <vtkRpCubeAxesActor2D.h>
#else
#include <vtkCubeAxesActor2D.h>
#endif
#include <vtkDataSetReader.h>
#include <vtkDataSetMapper.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>

#include "RpVtkRenderer.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Renderer::Renderer() :
    _needsRedraw(false),
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
     initAxes();
    _renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    _renderWindow->DoubleBufferOff();
    //_renderWindow->BordersOff();
    _renderWindow->SetSize(_windowWidth, _windowHeight);
    _renderWindow->OffScreenRenderingOn();
    _renderWindow->AddRenderer(_renderer);
    addColorMap("default", vtkLookupTable::New());
}

Renderer::~Renderer()
{
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
    _dataSets[id] = new DataSet;
}

/**
 * \brief Remove the PseudoColor mapping for the specified DataSet
 *
 * The underlying PseudoColor object is deleted, freeing its memory
 */
void Renderer::deletePseudoColor(DataSetId id)
{
    PseudoColorHashmap::iterator itr = _pseudoColors.find(id);
    if (itr == _pseudoColors.end())
        return;

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
    if (itr == _contours.end())
        return;

    Contour2D *contour = itr->second;
    if (contour->getActor())
        _renderer->RemoveActor(contour->getActor());
    delete contour;

    _contours.erase(itr);
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
    deletePseudoColor(id);
    deleteContour2D(id);

    DataSet *ds = getDataSet(id);
    if (ds) {
        delete ds;
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
    if (itr == _dataSets.end())
        return NULL;
    else
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
    } else
        return false;
}

/**
 * \brief Set inital properties on the 2D Axes
 */
void Renderer::initAxes()
{
#ifdef USE_CUSTOM_AXES
    _axesActor = vtkSmartPointer<vtkRpCubeAxesActor2D>::New();
#else
    _axesActor = vtkSmartPointer<vtkCubeAxesActor2D>::New();
#endif
    _axesActor->SetCamera(_renderer->GetActiveCamera());
    _renderer->AddActor(_axesActor);

    _axesActor->ZAxisVisibilityOff();
    // Don't offset labels at origin
    _axesActor->SetCornerOffset(0);
    _axesActor->SetFlyModeToClosestTriad();
    _axesActor->ScalingOff();
    //_axesActor->SetShowActualBounds(0);

    _axesActor->SetFontFactor(2);

    // Use "nice" range and number of ticks/labels
    _axesActor->GetXAxisActor2D()->AdjustLabelsOn();
    _axesActor->GetYAxisActor2D()->AdjustLabelsOn();

#ifdef USE_CUSTOM_AXES
    _axesActor->SetAxisTitleTextProperty(NULL);
    _axesActor->SetAxisLabelTextProperty(NULL);
    //_axesActor->GetXAxisActor2D()->SizeFontRelativeToAxisOn();
    _axesActor->GetXAxisActor2D()->GetTitleTextProperty()->BoldOn();
    _axesActor->GetXAxisActor2D()->GetTitleTextProperty()->ItalicOff();
    _axesActor->GetXAxisActor2D()->GetTitleTextProperty()->ShadowOn();
    _axesActor->GetXAxisActor2D()->GetLabelTextProperty()->BoldOff();
    _axesActor->GetXAxisActor2D()->GetLabelTextProperty()->ItalicOff();
    _axesActor->GetXAxisActor2D()->GetLabelTextProperty()->ShadowOff();

    //_axesActor->GetYAxisActor2D()->SizeFontRelativeToAxisOn();
    _axesActor->GetYAxisActor2D()->GetTitleTextProperty()->BoldOn();
    _axesActor->GetYAxisActor2D()->GetTitleTextProperty()->ItalicOff();
    _axesActor->GetYAxisActor2D()->GetTitleTextProperty()->ShadowOn();
    _axesActor->GetYAxisActor2D()->GetLabelTextProperty()->BoldOff();
    _axesActor->GetYAxisActor2D()->GetLabelTextProperty()->ItalicOff();
    _axesActor->GetYAxisActor2D()->GetLabelTextProperty()->ShadowOff();
#else
    _axesActor->GetAxisTitleTextProperty()->BoldOn();
    _axesActor->GetAxisTitleTextProperty()->ItalicOff();
    _axesActor->GetAxisTitleTextProperty()->ShadowOn();
    _axesActor->GetAxisLabelTextProperty()->BoldOff();
    _axesActor->GetAxisLabelTextProperty()->ItalicOff();
    _axesActor->GetAxisLabelTextProperty()->ShadowOff();
#endif
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
        deletePseudoColor(id);
    }
    PseudoColor *pc = new PseudoColor();
    _pseudoColors[id] = pc;

    pc->setDataSet(ds);

    _renderer->AddActor(pc->getActor());

    double actorBounds[6];
    pc->getActor()->GetBounds(actorBounds);

#ifdef DEBUG
    TRACE("Actor Bounds: %.6e, %.6e, %.6e, %.6e, %.6e, %.6e",
          actorBounds[0],
          actorBounds[1],
          actorBounds[2],
          actorBounds[3],
          actorBounds[4],
          actorBounds[5]);
#endif
    initCamera(actorBounds);
    _needsRedraw = true;
}

/**
 * \brief Get the PseudoColor associated with the specified DataSet
 */
PseudoColor *Renderer::getPseudoColor(DataSetId id)
{
    PseudoColorHashmap::iterator itr = _pseudoColors.find(id);

    if (itr == _pseudoColors.end())
        return NULL;
    else
        return itr->second;
}

/**
 * \brief Turn on/off rendering of all axes
 */
void Renderer::setAxesVisibility(bool state)
{
    if (_axesActor != NULL) {
        _axesActor->SetVisibility((state ? 1 : 0));
        _needsRedraw = true;
    }
}

/**
 * \brief Turn on/off rendering of the specified axis
 */
void Renderer::setAxisVisibility(Axis axis, bool state)
{
    if (_axesActor != NULL) {
        if (axis == X_AXIS) {
            _axesActor->SetXAxisVisibility((state ? 1 : 0));
        } else if (axis == Y_AXIS) {
            _axesActor->SetYAxisVisibility((state ? 1 : 0));
        } else if (axis == Z_AXIS) {
            _axesActor->SetZAxisVisibility((state ? 1 : 0));
        }
        _needsRedraw = true;
    }
}

/**
 * \brief Add a color map for use in the Renderer
 */
void Renderer::addColorMap(ColorMapId id, vtkLookupTable *lut)
{
    if (lut == NULL) {
        lut = vtkLookupTable::New();
    }
    _colorMaps[id] = lut;
}

/**
 * \brief Return the vtkLookupTable associated with the colormap key given
 */
vtkLookupTable *Renderer::getColorMap(ColorMapId id)
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
        _legendRenderer->AddActor(_scalarBarActor);
    }
    _scalarBarActor->SetLookupTable(getColorMap(id));
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
 * \brief Associate an existing named color map with a DataSet
 */
void Renderer::setPseudoColorColorMap(DataSetId id, ColorMapId colorMapId)
{
    PseudoColor *pc = getPseudoColor(id);
    if (pc) {
        pc->setLookupTable(getColorMap(colorMapId));
        _needsRedraw = true;
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
 * \brief Create a new Contour2D and associate it with the named DataSet
 */
void Renderer::addContour2D(DataSetId id)
{
    DataSet *ds = getDataSet(id);
    if (ds == NULL)
        return;

    if (getContour2D(id))
        deleteContour2D(id);

    Contour2D *contour = new Contour2D();
    _contours[id] = contour;

    contour->setDataSet(ds);

    _renderer->AddActor(contour->getActor());
}

/**
 * \brief Get the Contour2D associated with a named DataSet
 */
Contour2D *Renderer::getContour2D(DataSetId id)
{
    Contour2DHashmap::iterator itr = _contours.find(id);

    if (itr == _contours.end())
        return NULL;
    else
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
 * \brief Resize the render window (image size for renderings)
 */
void Renderer::setWindowSize(int width, int height)
{
    _windowWidth = width;
    _windowHeight = height;
    _renderWindow->SetSize(_windowWidth, _windowHeight);
#ifdef notdef
    setZoomRegion(_imgWorldOrigin[0], _imgWorldOrigin[1],
                 _imgWorldDims[0], _imgWorldDims[1]);
#endif
    _needsRedraw = true;
}

/**
 * \brief Set the pan/zoom using a corner and dimensions in world coordinates
 * 
 * x,y - bottom left corner in world coords
 */
void Renderer::setZoomRegion(double x, double y, double width, double height)
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

    _axesActor->SetBounds(_imgWorldOrigin[0], _imgWorldOrigin[0] + _imgWorldDims[0],
                          _imgWorldOrigin[1], _imgWorldOrigin[1] + _imgWorldDims[1], 0, 0);

#ifdef DEBUG
    printCameraInfo(camera);
#endif

    _needsRedraw = true;
}

/**
 * \brief Initialize the camera zoom region to include the bounding volume given
 */
void Renderer::initCamera(double bounds[6])
{
    _renderer->ResetCamera();
    setZoomRegion(bounds[0], bounds[2], bounds[1] - bounds[0], bounds[3] - bounds[2]);
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
}

/**
 * \brief Turn on/off rendering of the specified DataSet's associated graphics objects
 */
void Renderer::setVisibility(DataSetId id, bool state)
{
    setPseudoColorVisibility(id, state);
    setContourVisibility(id, state);
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
 * \brief Cause the rendering to render a new image if needed
 *
 * The _needsRedraw flag indicates if a state change has occured since
 * the last rendered frame
 */
bool Renderer::render()
{
    if (_needsRedraw) {
        for (PseudoColorHashmap::iterator itr = _pseudoColors.begin();
             itr != _pseudoColors.end(); ++itr) {
            itr->second->setClippingPlanes(_clippingPlanes);
        }
        for (Contour2DHashmap::iterator itr = _contours.begin();
             itr != _contours.end(); ++itr) {
            itr->second->setClippingPlanes(_clippingPlanes);
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
