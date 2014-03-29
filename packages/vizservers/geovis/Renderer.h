/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_RENDERER_H
#define GEOVIS_RENDERER_H

#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <typeinfo>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Image>
#include <osg/TransferFunction>
#include <osgViewer/Viewer>
#include <osgGA/StateSetManipulator>

#include <osgEarth/Map>
#include <osgEarth/Viewpoint>
#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>
#include <osgEarth/ModelLayer>
#include <osgEarth/TileSource>
#include <osgEarth/ModelSource>
#include <osgEarth/GeoData>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/MouseCoordsTool>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/VerticalScale>

#include "Types.h"
#include "Trace.h"

// Controls if TGA format is sent to client
//#define RENDER_TARGA
#define TARGA_BYTES_PER_PIXEL 3

namespace GeoVis {

class ScreenCaptureCallback : public osg::Camera::DrawCallback
{
public:
    ScreenCaptureCallback() :
        osg::Camera::DrawCallback()
    {
        _image = new osg::Image;
    }

    virtual void operator()(osg::RenderInfo &renderInfo) const
    {
        TRACE("Enter ScreenCaptureCallback");
        int width, height;
        if (renderInfo.getCurrentCamera() == NULL) {
            ERROR("No camera");
            return;
        }
        if (renderInfo.getCurrentCamera()->getViewport() == NULL) {
            ERROR("No viewport");
            return;
        }
        width = (int)renderInfo.getCurrentCamera()->getViewport()->width();
        height = (int)renderInfo.getCurrentCamera()->getViewport()->height();
        TRACE("readPixels: %d x %d", width, height);
#ifdef RENDER_TARGA
        _image->readPixels(0, 0, width, height,
                           GL_BGR, GL_UNSIGNED_BYTE);
#else
        _image->readPixels(0, 0, width, height,
                           GL_RGB, GL_UNSIGNED_BYTE);
#endif
    }

    osg::Image *getImage()
    {
        return _image.get();
    }

private:
    osg::ref_ptr<osg::Image> _image;
};

class MouseCoordsCallback : public osgEarth::Util::MouseCoordsTool::Callback
{
public:
    MouseCoordsCallback() :
        osgEarth::Util::MouseCoordsTool::Callback(),
        _havePoint(false)
    {}

    void set(const osgEarth::GeoPoint& p, osg::View *view, osgEarth::MapNode *mapNode)
    {
        TRACE("%g %g %g", p.x(), p.y(), p.z());
        _pt = p;
        _havePoint = true;
    }

    void reset(osg::View *view, osgEarth::MapNode *mapNode)
    {
        TRACE("Out of range");
        // Out of range click
        _havePoint = false;
    }

    bool report(double *x, double *y, double *z)
    {
        if (_havePoint) {
            *x = _pt.x();
            *y = _pt.y();
            *z = _pt.z();
            _havePoint = false;
            return true;
        }
        return false;
    }

private:
    bool _havePoint;
    osgEarth::GeoPoint _pt;
};

/**
 * \brief GIS Renderer
 */
class Renderer
{
public:
    typedef std::string ColorMapId;

    Renderer();
    virtual ~Renderer();

    // Colormaps

    void addColorMap(const ColorMapId& id, osg::TransferFunction1D *xfer);

    void deleteColorMap(const ColorMapId& id);

    void setColorMapNumberOfTableEntries(const ColorMapId& id, int numEntries);

    // Scene

    void loadEarthFile(const char *path);

    void resetMap(osgEarth::MapOptions::CoordinateSystemType type,
                  const char *profile = NULL,
                  double bounds[4] = NULL);

    void clearMap();

    // Map options

    void setViewerLightType(osg::View::LightingMode mode);

    void setLighting(bool state);

    void setTerrainLighting(bool state);

    void setTerrainVerticalScale(double scale);

    void setTerrainWireframe(bool state);

    // Image raster layers

    int getNumImageLayers() const
    {
        return (_map.valid() ? _map->getNumImageLayers() : 0);
    }

    void addImageLayer(const char *name, osgEarth::TileSourceOptions& opts,
                       bool makeShared = false, bool visible = true);

    void removeImageLayer(const char *name);

    void moveImageLayer(const char *name, unsigned int pos);

    void setImageLayerOpacity(const char *name, double opacity);

    void setImageLayerVisibility(const char *name, bool state);

    void addColorFilter(const char *name, const char *shader);

    void removeColorFilter(const char *name, int idx = -1);

    // Elevation raster layers

    int getNumElevationLayers() const
    {
        return (_map.valid() ? _map->getNumElevationLayers() : 0);
    }

    void addElevationLayer(const char *name, osgEarth::TileSourceOptions& opts);

    void removeElevationLayer(const char *name);

    void moveElevationLayer(const char *name, unsigned int pos);

    void setElevationLayerVisibility(const char *name, bool state);

    // Model layers

    int getNumModelLayers() const
    {
        return (_map.valid() ? _map->getNumModelLayers() : 0);
    }

    void addModelLayer(const char *name, osgEarth::ModelSourceOptions& opts);

    void removeModelLayer(const char *name);

    void moveModelLayer(const char *name, unsigned int pos);

    void setModelLayerOpacity(const char *name, double opacity);

    void setModelLayerVisibility(const char *name, bool state);

    // Render window

    void setWindowSize(int width, int height);

    int getWindowWidth() const
    {
        return _windowWidth;
    }

    int getWindowHeight() const
    {
        return _windowHeight;
    }

    // Camera

    osgEarth::Viewpoint getViewpoint();

    void setViewpoint(const osgEarth::Viewpoint& v, double durationSecs = 0.0);

    void resetCamera(bool resetOrientation = true);

    void setCameraOrientation(const double quat[4], bool absolute = true);

    void panCamera(double x, double y, bool absolute = false);

    void rotateCamera(double x, double y, bool absolute = false);

    void zoomCamera(double z, bool absolute = false);

    // Keyboard events

    void keyPress(int key);

    void keyRelease(int key);

    // Mouse events

    void setThrowingEnabled(bool state);

    void mouseClick(int button, double x, double y);

    void mouseDoubleClick(int button, double x, double y);

    void mouseDrag(int button, double x, double y);

    void mouseRelease(int button, double x, double y);

    void mouseMotion(double x, double y);

    void mouseScroll(int direction);

    // Rendering an image

    void setBackgroundColor(float color[3]);

    void eventuallyRender();

    bool render();

    osg::Image *getRenderedFrame();

    bool mapMouseCoords(float mouseX, float mouseY, osgEarth::GeoPoint &pt);

    bool getMousePoint(double *x, double *y, double *z)
    {
        return (_coordsCallback.valid() && _coordsCallback->report(x, y, z));
    }

    long getTimeout();

private:
    typedef std::tr1::unordered_map<ColorMapId, osg::ref_ptr<osg::TransferFunction1D> > ColorMapHashmap;

    void initViewer();

    void finalizeViewer();

    void initColorMaps();

    void initCamera();

    bool isPagerIdle();

    bool checkNeedToDoFrame();

    osgGA::EventQueue *getEventQueue();

    bool _needsRedraw;
    int _windowWidth, _windowHeight;
    float _bgColor[3];

    double _minFrameTime;
    double _lastFrameTime;

    ColorMapHashmap _colorMaps;

    osg::ref_ptr<osg::Node> _sceneRoot;
    osg::ref_ptr<osgEarth::MapNode> _mapNode;
    osg::ref_ptr<osgEarth::Map> _map;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<ScreenCaptureCallback> _captureCallback;
    osg::ref_ptr<osgEarth::Util::AutoClipPlaneCullCallback> _clipPlaneCullCallback;
    osg::ref_ptr<osgEarth::Util::MouseCoordsTool> _mouseCoordsTool;
    osg::ref_ptr<MouseCoordsCallback> _coordsCallback;
    osg::ref_ptr<osgEarth::Util::EarthManipulator> _manipulator;
    osg::ref_ptr<osgGA::StateSetManipulator> _stateManip;
    osg::ref_ptr<osgEarth::Util::VerticalScale> _verticalScale;
};

}

#endif
