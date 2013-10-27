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
#include <osgViewer/Viewer>

#include <osgEarth/Map>
#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>
#include <osgEarth/ModelLayer>
#include <osgEarth/TileSource>
#include <osgEarth/ModelSource>
#include <osgEarth/GeoData>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/MouseCoordsTool>

#include "Types.h"
#include "Trace.h"

// Controls if TGA format is sent to client
#define RENDER_TARGA
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
        TRACE("Enter");
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

    void set(const osgEarth::GeoPoint& p, osg::View* view, osgEarth::MapNode* mapNode)
    {
        // p.y(), p.x()
        _pt = p;
        _havePoint = true;
    }

    void reset(osg::View* view, osgEarth::MapNode* mapNode)
    {
        // Out of range click
        //_havePoint = false;
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
    Renderer();
    virtual ~Renderer();

    // Scene

    void loadEarthFile(const char *path);

    // Image raster layers

    void addImageLayer(const char *name, const osgEarth::TileSourceOptions& opts);

    void removeImageLayer(const char *name);

    void moveImageLayer(const char *name, unsigned int pos);

    void setImageLayerOpacity(const char *name, double opacity);

    void setImageLayerVisibility(const char *name, bool state);

    // Elevation raster layers

    void addElevationLayer(const char *name, const osgEarth::TileSourceOptions& opts);

    void removeElevationLayer(const char *name);

    void moveElevationLayer(const char *name, unsigned int pos);

    // Model layers

    void addModelLayer(const char *name, const osgEarth::ModelSourceOptions& opts);

    void removeModelLayer(const char *name);

    void moveModelLayer(const char *name, unsigned int pos);

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

    void resetCamera(bool resetOrientation = true);

    void setCameraOrientation(const double quat[4], bool absolute = true);

    void panCamera(double x, double y, bool absolute = false);

    void rotateCamera(double x, double y, bool absolute = false);

    void zoomCamera(double z, bool absolute = false);

    // Mouse events

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

    bool getMousePoint(double *x, double *y, double *z)
    {
        return _coordsCallback->report(x, y, z);
    }

private:
    void initCamera();

    bool _needsRedraw;
    int _windowWidth, _windowHeight;
    float _bgColor[3];

    osg::ref_ptr<osg::Node> _sceneRoot;
    osg::ref_ptr<osgEarth::MapNode> _mapNode;
    osg::ref_ptr<osgEarth::Map> _map;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<ScreenCaptureCallback> _captureCallback;
    osg::ref_ptr<osgEarth::Util::MouseCoordsTool> _mouseCoordsTool;
    osg::ref_ptr<MouseCoordsCallback> _coordsCallback;
    osg::ref_ptr<osgEarth::Util::EarthManipulator> _manipulator;
};

}

#endif
