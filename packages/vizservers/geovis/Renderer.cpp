/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
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

#include <osgGA/StateSetManipulator>

#include <osgEarth/Version>
#include <osgEarth/MapNode>
#include <osgEarth/TerrainLayer>
#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>
#include <osgEarth/ModelLayer>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/MouseCoordsTool>

#include "Renderer.h"
#include "Trace.h"

#define MSECS_ELAPSED(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3 + (double)((t2).tv_usec - (t1).tv_usec)/1.0e+3)

using namespace GeoVis;

Renderer::Renderer() :
    _needsRedraw(true),
    _windowWidth(500),
    _windowHeight(500)
{
    _bgColor[0] = 0;
    _bgColor[1] = 0;
    _bgColor[2] = 0;
    _minFrameTime = 1.0/30.0;
    _lastFrameTime = _minFrameTime;
    _viewer = new osgViewer::Viewer();
    _viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    _viewer->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(false, false);
    _viewer->setReleaseContextAtEndOfFrameHint(false);
    setBackgroundColor(_bgColor);
    _captureCallback = new ScreenCaptureCallback();
    _viewer->getCamera()->setPostDrawCallback(_captureCallback.get());
    osgEarth::MapOptions mapOpts;
    mapOpts.coordSysType() = osgEarth::MapOptions::CSTYPE_PROJECTED;
    mapOpts.profile() = osgEarth::ProfileOptions("global-geodetic");
    osgEarth::Map *map = new osgEarth::Map(mapOpts);
    _map = map;
    osgEarth::MapNodeOptions mapNodeOpts;
    mapNodeOpts.enableLighting() = false;
    osgEarth::MapNode *mapNode = new osgEarth::MapNode(map, mapNodeOpts);
    _mapNode = mapNode;
    _sceneRoot = mapNode;
    _viewer->setSceneData(_sceneRoot.get());
    _manipulator = new osgEarth::Util::EarthManipulator;
    _viewer->setCameraManipulator(_manipulator.get());
    _viewer->addEventHandler(new osgGA::StateSetManipulator(_viewer->getCamera()->getOrCreateStateSet()));
    _coordsCallback = new MouseCoordsCallback();
    _mouseCoordsTool = new osgEarth::Util::MouseCoordsTool(mapNode);
    _mouseCoordsTool->addCallback(_coordsCallback);
    _viewer->addEventHandler(_mouseCoordsTool);
    _viewer->getCamera()->setNearFarRatio(0.00002);
    _viewer->getCamera()->setSmallFeatureCullingPixelSize(-1.0f);
    _viewer->setUpViewInWindow(0, 0, _windowWidth, _windowHeight);
    _viewer->realize();
#ifdef DEBUG
    if (_viewer->getViewerStats() != NULL) {
        TRACE("Enabling stats");
        _viewer->getViewerStats()->collectStats("scene", true);
    }
#endif
#if 0
    osgViewer::ViewerBase::Windows windows;
    _viewer->getWindows(windows);
    if (windows.size() == 1) {
        windows[0]->setSyncToVBlank(false);
    } else {
        ERROR("Num windows: %lu", windows.size());
    }
#endif
}

osgGA::EventQueue *Renderer::getEventQueue()
{
    osgViewer::ViewerBase::Windows windows;
    _viewer->getWindows(windows);
    return windows[0]->getEventQueue();
}

Renderer::~Renderer()
{
    TRACE("Enter");

    TRACE("Leave");
}

void Renderer::loadEarthFile(const char *path)
{
    TRACE("Loading %s", path);
    osg::Node *node = osgDB::readNodeFile(path);
    if (node == NULL) {
        ERROR("Couldn't load %s", path);
        return;
    }
    osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
    if (mapNode == NULL) {
        ERROR("Couldn't find MapNode");
        return;
    } else {
        _sceneRoot = node;
        _map = mapNode->getMap();
    }
    _mapNode = mapNode;
    if (_mouseCoordsTool.valid())
        _viewer->removeEventHandler(_mouseCoordsTool.get());
    _mouseCoordsTool = new osgEarth::Util::MouseCoordsTool(mapNode);
    _mouseCoordsTool->addCallback(_coordsCallback.get());
    _viewer->addEventHandler(_mouseCoordsTool.get());
    _viewer->setSceneData(_sceneRoot.get());
    _manipulator = new osgEarth::Util::EarthManipulator;
    _viewer->setCameraManipulator(_manipulator.get());
    _manipulator->setNode(NULL);
    _manipulator->setNode(_sceneRoot.get());
    _manipulator->computeHomePosition();
    _viewer->home();
    _needsRedraw = true;
}

void Renderer::resetMap(osgEarth::MapOptions::CoordinateSystemType type, const char *profile)
{
    TRACE("Restting map with type %d, profile %s", type, profile);

    osgEarth::MapOptions mapOpts;
    mapOpts.coordSysType() = type;
    if (profile != NULL) {
        mapOpts.profile() = osgEarth::ProfileOptions(profile);
    } else if (type == osgEarth::MapOptions::CSTYPE_PROJECTED) {
        mapOpts.profile() = osgEarth::ProfileOptions("global-geodetic");
    }
    osgEarth::Map *map = new osgEarth::Map(mapOpts);
    _map = map;
    osgEarth::MapNodeOptions mapNodeOpts;
    mapNodeOpts.enableLighting() = false;
    osgEarth::MapNode *mapNode = new osgEarth::MapNode(map, mapNodeOpts);
    _mapNode = mapNode;
    _sceneRoot = mapNode;
    if (_mouseCoordsTool.valid())
        _viewer->removeEventHandler(_mouseCoordsTool.get());
    _mouseCoordsTool = new osgEarth::Util::MouseCoordsTool(mapNode);
    _mouseCoordsTool->addCallback(_coordsCallback.get());
    _viewer->addEventHandler(_mouseCoordsTool.get());
    _viewer->setSceneData(_sceneRoot.get());
    _manipulator = new osgEarth::Util::EarthManipulator;
    _viewer->setCameraManipulator(_manipulator.get());
    _manipulator->setNode(NULL);
    _manipulator->setNode(_sceneRoot.get());
    _manipulator->computeHomePosition();
    _viewer->home();
    _needsRedraw = true;
}

bool Renderer::mapMouseCoords(float mouseX, float mouseY, osgEarth::GeoPoint& map)
{
    osg::Vec3d world;
    if (_mapNode->getTerrain()->getWorldCoordsUnderMouse(_viewer.get(), mouseX, mouseY, world)) {
        map.fromWorld(_mapNode->getMapSRS(), world);
        return true;
    }
    return false;
}

void Renderer::addImageLayer(const char *name, const osgEarth::TileSourceOptions& opts)
{
    osgEarth::ImageLayerOptions layerOpts(name, opts);
    _map->addImageLayer(new osgEarth::ImageLayer(layerOpts));
}

void Renderer::removeImageLayer(const char *name)
{
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    _map->removeImageLayer(layer);
}

void Renderer::moveImageLayer(const char *name, unsigned int pos)
{
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    _map->moveImageLayer(layer, pos);
}

void Renderer::setImageLayerOpacity(const char *name, double opacity)
{
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    layer->setOpacity(opacity);
}

void Renderer::setImageLayerVisibility(const char *name, bool state)
{
#if OSGEARTH_MIN_VERSION_REQUIRED(2, 4, 0)
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    layer->setVisible(state);
#endif
}

void Renderer::addElevationLayer(const char *name, const osgEarth::TileSourceOptions& opts)
{
    osgEarth::ElevationLayerOptions layerOpts(name, opts);
    _map->addElevationLayer(new osgEarth::ElevationLayer(layerOpts));
}

void Renderer::removeElevationLayer(const char *name)
{
    osgEarth::ElevationLayer *layer = _map->getElevationLayerByName(name);
    _map->removeElevationLayer(layer);
}

void Renderer::moveElevationLayer(const char *name, unsigned int pos)
{
    osgEarth::ElevationLayer *layer = _map->getElevationLayerByName(name);
    _map->moveElevationLayer(layer, pos);
}

void Renderer::addModelLayer(const char *name, const osgEarth::ModelSourceOptions& opts)
{
    osgEarth::ModelLayerOptions layerOpts(name, opts);
    _map->addModelLayer(new osgEarth::ModelLayer(layerOpts));
}

void Renderer::removeModelLayer(const char *name)
{
    osgEarth::ModelLayer *layer = _map->getModelLayerByName(name);
    _map->removeModelLayer(layer);
}

void Renderer::moveModelLayer(const char *name, unsigned int pos)
{
    osgEarth::ModelLayer *layer = _map->getModelLayerByName(name);
    _map->moveModelLayer(layer, pos);
}

/**
 * \brief Resize the render window (image size for renderings)
 */
void Renderer::setWindowSize(int width, int height)
{
    if (_windowWidth == width &&
        _windowHeight == height) {
        TRACE("No change");
        return;
    }

    TRACE("Setting window size to %dx%d", width, height);

    _windowWidth = width;
    _windowHeight = height;
    osgViewer::ViewerBase::Windows windows;
    _viewer->getWindows(windows);
    if (windows.size() == 1) {
        windows[0]->setWindowRectangle(0, 0, _windowWidth, _windowHeight);
    } else {
        ERROR("Num windows: %lu", windows.size());
    }
    _needsRedraw = true;
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
    _manipulator->setRotation(osg::Quat(quat[1], quat[2], quat[3], quat[0]));
    _needsRedraw = true;
}

/**
 * \brief Reset pan, zoom, clipping planes and optionally rotation
 *
 * \param[in] resetOrientation Reset the camera rotation/orientation also
 */
void Renderer::resetCamera(bool resetOrientation)
{
    TRACE("Enter: resetOrientation=%d", resetOrientation ? 1 : 0);
    _viewer->home();
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
    TRACE("Enter: %g %g, abs: %d",
          x, y, (absolute ? 1 : 0));

    _manipulator->pan(x, y);
    _needsRedraw = true;
}

void Renderer::rotateCamera(double x, double y, bool absolute)
{
    TRACE("Enter: %g %g, abs: %d",
          x, y, (absolute ? 1 : 0));

    _manipulator->rotate(x, y);
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
    TRACE("Enter: z: %g, abs: %d",
          z, (absolute ? 1 : 0));

    // FIXME: zoom here wants y mouse coords in normalized viewport coords

    _manipulator->zoom(0, z);
    _needsRedraw = true;
}

void Renderer::setThrowingEnabled(bool state)
{
    _manipulator->getSettings()->setThrowingEnabled(state);
}

void Renderer::mouseDoubleClick(int button, double x, double y)
{
    getEventQueue()->mouseDoubleButtonPress((float)x, (float)y, button);
    _needsRedraw = true;
}

void Renderer::mouseClick(int button, double x, double y)
{
    getEventQueue()->mouseButtonPress((float)x, (float)y, button);
    _needsRedraw = true;
}

void Renderer::mouseDrag(int button, double x, double y)
{
    getEventQueue()->mouseMotion((float)x, (float)y);
    _needsRedraw = true;
}

void Renderer::mouseRelease(int button, double x, double y)
{
    getEventQueue()->mouseButtonRelease((float)x, (float)y, button);
    _needsRedraw = true;
}

void Renderer::mouseMotion(double x, double y)
{
    //getEventQueue()->mouseMotion((float)x, (float)y);
    return;
    osgEarth::GeoPoint map;
    if (mapMouseCoords(x, y, map)) {
        _coordsCallback->set(map, _viewer.get(), _mapNode);
    } else {
        _coordsCallback->reset(_viewer.get(), _mapNode);
    }
}

void Renderer::mouseScroll(int direction)
{
    getEventQueue()->mouseScroll((direction > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN));
    _needsRedraw = true;
}

/**
 * \brief Set the RGB background color to render into the image
 */
void Renderer::setBackgroundColor(float color[3])
{
    _bgColor[0] = color[0];
    _bgColor[1] = color[1];
    _bgColor[2] = color[2];

    _viewer->getCamera()->setClearColor(osg::Vec4(color[0], color[1], color[2], 1));

    _needsRedraw = true;
}

/**
 * \brief Sets flag to trigger rendering next time render() is called
 */
void Renderer::eventuallyRender()
{
    _needsRedraw = true;
}

long Renderer::getTimeout()
{
    if (_lastFrameTime < _minFrameTime) {
        return (long)1000000.0*(_minFrameTime - _lastFrameTime);
    } else {
        return 0L;
    }
}

bool Renderer::isPagerIdle()
{
    //return (!_viewer->getDatabasePager()->requiresUpdateSceneGraph() &&
    //        !_viewer->getDatabasePager()->getRequestsInProgress());
    return !_viewer->getDatabasePager()->requiresUpdateSceneGraph();
}

/**
 * \brief Cause the rendering to render a new image if needed
 *
 * The _needsRedraw flag indicates if a state change has occured since
 * the last rendered frame
 */
bool Renderer::render()
{
    if (_needsRedraw ||
        //!isPagerIdle()
        _viewer->checkNeedToDoFrame()
        ) {
        TRACE("Enter needsRedraw=%d",  _needsRedraw ? 1 : 0);

        osg::Timer_t startFrameTick = osg::Timer::instance()->tick();
        TRACE("Before frame()");
        _viewer->frame();
        TRACE("After frame()");
        osg::Timer_t endFrameTick = osg::Timer::instance()->tick();
        _lastFrameTime = osg::Timer::instance()->delta_s(startFrameTick, endFrameTick);
        TRACE("Frame time: %g sec", _lastFrameTime);
#if 0
        if (frameTime < minFrameTime) {
            TRACE("Sleeping for %g secs", minFrameTime-frameTime);
            OpenThreads::Thread::microSleep(static_cast<unsigned int>(1000000.0*(minFrameTime-frameTime)));
        }
#endif
#ifdef WANT_TRACE
        if (_viewer->getViewerStats() != NULL) {
            _viewer->getViewerStats()->report(std::cerr, _viewer->getViewerStats()->getLatestFrameNumber());
        }
#endif
        _needsRedraw = false;
        return true;
    } else
        return false;
}

/**
 * \brief Read back the rendered framebuffer image
 */
osg::Image *Renderer::getRenderedFrame()
{
    return _captureCallback->getImage();
}
