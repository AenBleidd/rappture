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
#include <cstdlib>

#include <GL/gl.h>

#ifdef WANT_TRACE
#include <sys/time.h>
#endif

#include <osgGA/StateSetManipulator>
#include <osgGA/GUIEventAdapter>
#include <osgViewer/ViewerEventHandlers>

#include <osgEarth/Version>
#include <osgEarth/MapNode>
#include <osgEarth/Bounds>
#include <osgEarth/Profile>
#include <osgEarth/Viewpoint>
#include <osgEarth/TerrainLayer>
#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>
#include <osgEarth/ModelLayer>
#include <osgEarthUtil/EarthManipulator>
#if OSGEARTH_MIN_VERSION_REQUIRED(2, 5, 1)
#include <osgEarthUtil/Sky>
#else
#include <osgEarthUtil/SkyNode>
#endif
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/MouseCoordsTool>
#include <osgEarthUtil/UTMGraticule>
#include <osgEarthUtil/MGRSGraticule>
#include <osgEarthUtil/GeodeticGraticule>
#include <osgEarthUtil/LatLongFormatter>
#include <osgEarthUtil/GLSLColorFilter>
#include <osgEarthUtil/VerticalScale>
#include <osgEarthDrivers/gdal/GDALOptions>
#include <osgEarthDrivers/engine_mp/MPTerrainEngineOptions>

#include "Renderer.h"
#include "Trace.h"

#define MSECS_ELAPSED(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3 + (double)((t2).tv_usec - (t1).tv_usec)/1.0e+3)

#define BASE_IMAGE "/usr/share/osgearth/data/world.tif"

using namespace GeoVis;

Renderer::Renderer() :
    _needsRedraw(false),
    _windowWidth(500),
    _windowHeight(500)
{
    _bgColor[0] = 0;
    _bgColor[1] = 0;
    _bgColor[2] = 0;
    _minFrameTime = 1.0/30.0;
    _lastFrameTime = _minFrameTime;

    char *base = getenv("MAP_BASE_URI");
    if (base != NULL) {
        _baseURI = base;
        TRACE("Setting base URI: %s", _baseURI.c_str());
    }

#if 0
    initViewer();

    osgEarth::MapOptions mapOpts;
    mapOpts.coordSysType() = osgEarth::MapOptions::CSTYPE_PROJECTED;
    mapOpts.profile() = osgEarth::ProfileOptions("global-mercator");
    osgEarth::Map *map = new osgEarth::Map(mapOpts);
    _map = map;
    osgEarth::Drivers::GDALOptions bopts;
    bopts.url() = BASE_IMAGE;
    addImageLayer("base", bopts);
    osgEarth::Drivers::MPTerrainEngineOptions mpOpt;
    // Set background layer color
    mpOpt.color() = osg::Vec4(1, 1, 1, 1);
    //mpOpt.minLOD() = 1;
    osgEarth::MapNodeOptions mapNodeOpts(mpOpt);
    mapNodeOpts.enableLighting() = false;
    osgEarth::MapNode *mapNode = new osgEarth::MapNode(map, mapNodeOpts);
    _mapNode = mapNode;
    _sceneRoot = new osg::Group;
    _sceneRoot->addChild(mapNode);
    _viewer->setSceneData(_sceneRoot.get());

    initEarthManipulator();
    initMouseCoordsTool();

    finalizeViewer();
#endif

#ifdef DEBUG
    if (_viewer.valid() && _viewer->getViewerStats() != NULL) {
        TRACE("Enabling stats");
        _viewer->getViewerStats()->collectStats("scene", true);
    }
#endif
#if 0
    if (_viewer.valid()) {
        osgViewer::ViewerBase::Windows windows;
        _viewer->getWindows(windows);
        if (windows.size() == 1) {
            windows[0]->setSyncToVBlank(false);
        } else {
            ERROR("Num windows: %lu", windows.size());
        }
    }
#endif
}

osgGA::EventQueue *Renderer::getEventQueue()
{
    if (_viewer.valid()) {
        osgViewer::ViewerBase::Windows windows;
        _viewer->getWindows(windows);
        if (windows.size() > 0) {
            return windows[0]->getEventQueue();
        }
    }
    return NULL;
}

Renderer::~Renderer()
{
    TRACE("Enter");

    TRACE("Leave");
}

void Renderer::initColorMaps()
{
    if (!_colorMaps.empty())
        return;

    osg::TransferFunction1D *defaultColorMap = new osg::TransferFunction1D;
    defaultColorMap->allocate(256);
    defaultColorMap->setColor(0.00, osg::Vec4f(0,0,1,1), false);
    defaultColorMap->setColor(0.25, osg::Vec4f(0,1,1,1), false);
    defaultColorMap->setColor(0.50, osg::Vec4f(0,1,0,1), false);
    defaultColorMap->setColor(0.75, osg::Vec4f(1,1,0,1), false);
    defaultColorMap->setColor(1.00, osg::Vec4f(1,0,0,1), false);
    defaultColorMap->updateImage();
    addColorMap("default", defaultColorMap);
    osg::TransferFunction1D *defaultGrayColorMap = new osg::TransferFunction1D;
    defaultGrayColorMap->allocate(256);
    defaultGrayColorMap->setColor(0, osg::Vec4f(0,0,0,1), false);
    defaultGrayColorMap->setColor(1, osg::Vec4f(1,1,1,1), false);
    defaultGrayColorMap->updateImage();
    addColorMap("grayDefault", defaultGrayColorMap);
}

void Renderer::addColorMap(const ColorMapId& id, osg::TransferFunction1D *xfer)
{
    _colorMaps[id] = xfer;
#if 0
    osgEarth::Drivers::GDALOptions opts;
    char *url =  Tcl_GetString(objv[4]);
    std::ostringstream oss;
    oss << "_cmap_" << id;

    opts.url() = url;

    addImageLayer(oss.str().c_str(), opts);
#endif
}

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
        if (itr->first.compare("default") == 0 ||
            itr->first.compare("grayDefault") == 0) {
            if (id.compare("all") != 0) {
                WARN("Cannot delete a default color map");
            }
            continue;
        }

        TRACE("Deleting ColorMap %s", itr->first.c_str());
        itr = _colorMaps.erase(itr);
    } while (doAll && itr != _colorMaps.end());
}

void Renderer::setColorMapNumberOfTableEntries(const ColorMapId& id,
                                               int numEntries)
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
        itr->second->allocate(numEntries);
        itr->second->updateImage();
    } while (doAll && ++itr != _colorMaps.end());
}

void Renderer::initViewer() {
    if (_viewer.valid())
        return;
    _viewer = new osgViewer::Viewer();
    _viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    _viewer->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(false, false);
    _viewer->setReleaseContextAtEndOfFrameHint(false);
    //_viewer->setLightingMode(osg::View::SKY_LIGHT);
    _viewer->getCamera()->setClearColor(osg::Vec4(_bgColor[0], _bgColor[1], _bgColor[2], 1));
    _viewer->getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _viewer->getCamera()->setNearFarRatio(0.00002);
    _viewer->getCamera()->setSmallFeatureCullingPixelSize(-1.0f);
    _stateManip = new osgGA::StateSetManipulator(_viewer->getCamera()->getOrCreateStateSet());
    _viewer->addEventHandler(_stateManip);
    //_viewer->addEventHandler(new osgViewer::StatsHandler());
}

void Renderer::finalizeViewer() {
    initViewer();
    if (!_viewer->isRealized()) {
#ifdef USE_OFFSCREEN_RENDERING
#ifdef USE_PBUFFER
        osg::ref_ptr<osg::GraphicsContext> pbuffer;
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->x = 0;
        traits->y = 0;
        traits->width = _windowWidth;
        traits->height = _windowHeight;
        traits->red = 8;
        traits->green = 8;
        traits->blue = 8;
        traits->alpha = 8;
        traits->windowDecoration = false;
        traits->pbuffer = true;
        traits->doubleBuffer = true;
        traits->sharedContext = 0;

        pbuffer = osg::GraphicsContext::createGraphicsContext(traits.get());
        if (pbuffer.valid()) {
            TRACE("Pixel buffer has been created successfully.");
        } else {
            ERROR("Pixel buffer has not been created successfully.");
        }
        osg::Camera *camera = new osg::Camera;
        camera->setGraphicsContext(pbuffer.get());
        camera->setViewport(new osg::Viewport(0, 0, _windowWidth, _windowHeight));
        GLenum buffer = pbuffer->getTraits()->doubleBuffer ? GL_BACK : GL_FRONT;
        camera->setDrawBuffer(buffer);
        camera->setReadBuffer(buffer);
        _captureCallback = new ScreenCaptureCallback();
        camera->setFinalDrawCallback(_captureCallback.get());
        _viewer->addSlave(camera, osg::Matrixd(), osg::Matrixd());
#else
        _viewer->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        osg::Texture2D* texture2D = new osg::Texture2D;
        texture2D->setTextureSize(_windowWidth, _windowHeight);
        texture2D->setInternalFormat(GL_RGBA);
        texture2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
        texture2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);

        _viewer->getCamera()->setImplicitBufferAttachmentMask(0, 0);
        _viewer->getCamera()->attach(osg::Camera::COLOR_BUFFER0, texture2D);
        //_viewer->getCamera()->attach(osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);
        _viewer->getCamera()->attach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, GL_DEPTH24_STENCIL8_EXT);
        _captureCallback = new ScreenCaptureCallback(texture2D);
        _viewer->getCamera()->setFinalDrawCallback(_captureCallback.get());
        _viewer->setUpViewInWindow(0, 0, _windowWidth, _windowHeight);
#endif
#else
        _captureCallback = new ScreenCaptureCallback();
        _viewer->getCamera()->setFinalDrawCallback(_captureCallback.get());
        //_viewer->getCamera()->getDisplaySettings()->setDoubleBuffer(false);
        _viewer->setUpViewInWindow(0, 0, _windowWidth, _windowHeight);
#endif
        _viewer->realize();
        initColorMaps();
    }
}

void Renderer::setGraticule(bool enable, GraticuleType type)
{
    if (!_mapNode.valid() || !_sceneRoot.valid())
        return;
    if (enable) {
        if (_graticule.valid()) {
            _sceneRoot->removeChild(_graticule.get());
            _graticule = NULL;
        }
        switch (type) {
        case GRATICULE_UTM: {
            osgEarth::Util::UTMGraticule *gr = new osgEarth::Util::UTMGraticule(_mapNode.get());
            _sceneRoot->addChild(gr);
            _graticule = gr;
        }
            break;
        case GRATICULE_MGRS: {
            osgEarth::Util::MGRSGraticule *gr = new osgEarth::Util::MGRSGraticule(_mapNode.get());
            _sceneRoot->addChild(gr);
            _graticule = gr;
        }
            break;
        case GRATICULE_GEODETIC:
        default:
            osgEarth::Util::GeodeticGraticule *gr = new osgEarth::Util::GeodeticGraticule(_mapNode.get());
            osgEarth::Util::GeodeticGraticuleOptions opt = gr->getOptions();
            opt.lineStyle()->getOrCreate<osgEarth::Symbology::LineSymbol>()->stroke()->color().set(1,0,0,1);
            gr->setOptions(opt);
            _sceneRoot->addChild(gr);
            _graticule = gr;
        }
    } else if (_graticule.valid()) {
        _sceneRoot->removeChild(_graticule.get());
        _graticule = NULL;
    }
    _needsRedraw = true;
}

void Renderer::initMouseCoordsTool()
{
    if (_mouseCoordsTool.valid()) {
        _viewer->removeEventHandler(_mouseCoordsTool.get());
    }
    _mouseCoordsTool = new osgEarth::Util::MouseCoordsTool(_mapNode.get());

    if (!_coordsCallback.valid()) {
        osgEarth::Util::Controls::LabelControl *readout =
            new osgEarth::Util::Controls::LabelControl("", 12.0f);
        osgEarth::Util::Controls::ControlCanvas::get(_viewer.get(), true)->addControl(readout);
        osgEarth::Util::LatLongFormatter *formatter =
            new osgEarth::Util::LatLongFormatter();
        formatter->setPrecision(5);
        _coordsCallback = new MouseCoordsCallback(readout, formatter);
    }
    _mouseCoordsTool->addCallback(_coordsCallback.get());
    _viewer->addEventHandler(_mouseCoordsTool.get());
}

void Renderer::initEarthManipulator()
{
    _manipulator = new osgEarth::Util::EarthManipulator;
#if 1
    osgEarth::Util::EarthManipulator::Settings *settings = _manipulator->getSettings();
    settings->bindMouse(osgEarth::Util::EarthManipulator::ACTION_ROTATE,
                        osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON,
                        osgGA::GUIEventAdapter::MODKEY_ALT);
    osgEarth::Util::EarthManipulator::ActionOptions options;
    options.clear();
    options.add(osgEarth::Util::EarthManipulator::OPTION_CONTINUOUS, true);
    settings->bindMouse(osgEarth::Util::EarthManipulator::ACTION_ZOOM,
                        osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON,
                        osgGA::GUIEventAdapter::MODKEY_ALT, options);
    _manipulator->applySettings(settings);
#endif
    _viewer->setCameraManipulator(_manipulator.get());
    _manipulator->setNode(NULL);
    _manipulator->setNode(_sceneRoot.get());
    _manipulator->computeHomePosition();
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
        initViewer();
        _sceneRoot = new osg::Group;
        _sceneRoot->addChild(node);
        _map = mapNode->getMap();
    }
    _mapNode = mapNode;

    if (_clipPlaneCullCallback.valid()) {
        _viewer->getCamera()->removeCullCallback(_clipPlaneCullCallback.get());
        _clipPlaneCullCallback = NULL;
    }
    if (_map->isGeocentric()) {
        _clipPlaneCullCallback = new osgEarth::Util::AutoClipPlaneCullCallback(mapNode);
        _viewer->getCamera()->addCullCallback(_clipPlaneCullCallback.get());
    }
    _viewer->setSceneData(_sceneRoot.get());

    initMouseCoordsTool();
    initEarthManipulator();
    
    _viewer->home();
    finalizeViewer();
    _needsRedraw = true;
}

void Renderer::resetMap(osgEarth::MapOptions::CoordinateSystemType type,
                        const char *profile,
                        double bounds[4])
{
    TRACE("Restting map with type %d, profile %s", type, profile);

    osgEarth::MapOptions mapOpts;
    mapOpts.coordSysType() = type;
    if (profile != NULL) {
        if (bounds != NULL) {
            mapOpts.profile() = osgEarth::ProfileOptions();
            if (strcmp(profile, "geodetic") == 0) {
                mapOpts.profile()->srsString() = "epsg:4326";
            } else if (strcmp(profile, "spherical-mercator") == 0) {
                // Projection used by Google/Bing/OSM
                // aka epsg:900913 meters in x/y
                // aka WGS84 Web Mercator (Auxiliary Sphere)
                // X/Y: -20037508.34m to 20037508.34m
                mapOpts.profile()->srsString() = "epsg:3857";
            } else {
                mapOpts.profile()->srsString() = profile;
            }
            TRACE("Setting profile bounds: %g %g %g %g",
                  bounds[0], bounds[1], bounds[2], bounds[3]);
            mapOpts.profile()->bounds() = 
                osgEarth::Bounds(bounds[0], bounds[1], bounds[2], bounds[3]);
        } else {
            mapOpts.profile() = osgEarth::ProfileOptions(profile);
        }
    } else if (type == osgEarth::MapOptions::CSTYPE_PROJECTED) {
        mapOpts.profile() = osgEarth::ProfileOptions("global-mercator");
    }

    initViewer();

    //mapOpts.referenceURI() = _baseURI;
    osgEarth::Map *map = new osgEarth::Map(mapOpts);
    _map = map;
    osgEarth::Drivers::GDALOptions bopts;
    bopts.url() = BASE_IMAGE;
    addImageLayer("base", bopts);
    osgEarth::Drivers::MPTerrainEngineOptions mpOpt;
    // Set background layer color
    mpOpt.color() = osg::Vec4(1, 1, 1, 1);
    //mpOpt.minLOD() = 1;
    // Sets shader uniform for terrain renderer (config var defaults to false)
    mpOpt.enableLighting() = false;
    osgEarth::MapNodeOptions mapNodeOpts(mpOpt);
    // Sets GL_LIGHTING state in MapNode's StateSet (config var defaults to true)
    mapNodeOpts.enableLighting() = true;
    osgEarth::MapNode *mapNode = new osgEarth::MapNode(map, mapNodeOpts);
    _mapNode = mapNode;
    if (_map->isGeocentric()) {
#if OSGEARTH_MIN_VERSION_REQUIRED(2, 5, 1)
        osgEarth::Util::SkyNode *sky = new osgEarth::Util::SkyNode::create(mapNode);
        sky->addChild(mapNode.get());
        _sceneRoot = sky;
#else
#if 0
        // XXX: Crashes
        osgEarth::Util::SkyNode *sky = new osgEarth::Util::SkyNode(map);
        sky->addChild(mapNode.get());
        _sceneRoot = sky;
#else
        _sceneRoot = new osg::Group();
        _sceneRoot->addChild(_mapNode.get());
#endif
#endif
    } else {
        _sceneRoot = new osg::Group();
        _sceneRoot->addChild(_mapNode.get());
    }

    if (_clipPlaneCullCallback.valid()) {
        _viewer->getCamera()->removeCullCallback(_clipPlaneCullCallback.get());
        _clipPlaneCullCallback = NULL;
    }
    if (_map->isGeocentric()) {
        _clipPlaneCullCallback = new osgEarth::Util::AutoClipPlaneCullCallback(mapNode);
        _viewer->getCamera()->addCullCallback(_clipPlaneCullCallback.get());
    }
    _viewer->setSceneData(_sceneRoot.get());
    initMouseCoordsTool();
    //_viewer->setSceneData(_sceneRoot.get());
    initEarthManipulator();
    _viewer->home();

    finalizeViewer();
    _needsRedraw = true;
}

void Renderer::clearMap()
{
    if (_map.valid()) {
        _map->clear();
        _needsRedraw = true;
    }
}

void Renderer::setLighting(bool state)
{
    if (_mapNode.valid()) {
        TRACE("Setting lighting: %d", state ? 1 : 0);
        _mapNode->getOrCreateStateSet()
            ->setMode(GL_LIGHTING, state ? 1 : 0);
        _needsRedraw = true;
    }
}

void Renderer::setViewerLightType(osg::View::LightingMode mode)
{
    if (_viewer.valid()) {
        _viewer->setLightingMode(mode);
        _needsRedraw = true;
    }
}

void Renderer::setTerrainVerticalScale(double scale)
{
    if (_mapNode.valid()) {
        if (!_verticalScale.valid()) {
            _verticalScale = new osgEarth::Util::VerticalScale;
            _mapNode->getTerrainEngine()->addEffect(_verticalScale);
        }
        _verticalScale->setScale(scale);
        _needsRedraw = true;
    }
}

void Renderer::setTerrainLighting(bool state)
{
#if 1
    if (!_mapNode.valid()) {
        ERROR("No map node");
        return;
    }
    // XXX: HACK alert
    // Find the terrain engine container (might be above one or more decorators)
    osg::Group *group = _mapNode->getTerrainEngine();
    while (group->getParent(0) != NULL && group->getParent(0) != _mapNode.get()) {
        group = group->getParent(0);
    }
    if (group != NULL && group->getParent(0) == _mapNode.get()) {
        TRACE("Setting terrain lighting: %d", state ? 1 : 0);
        if (group->getOrCreateStateSet()->getUniform("oe_mode_GL_LIGHTING") != NULL) {
            group->getStateSet()->getUniform("oe_mode_GL_LIGHTING")->set(state);
        } else {
            ERROR("Can't get terrain lighting uniform");
        }
    } else {
        ERROR("Can't find terrain engine container");
    }
#else
    if (_stateManip.valid()) {
        _stateManip->setLightingEnabled(state);
    }
#endif
    _needsRedraw = true;
}

void Renderer::setTerrainWireframe(bool state)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
#if 0
    if (!_mapNode.valid()) {
        ERROR("No map node");
        return;
    }
    TRACE("Setting terrain wireframe: %d", state ? 1 : 0);
    osg::StateSet *state = _mapNode->getOrCreateStateSet();
    osg::PolygonMode *pmode = dynamic_cast< osg::PolygonMode* >(state->getAttribute(osg::StateAttribute::POLYGONMODE));
    if (pmode == NULL) {
        pmode = new osg::PolygonMode;
        state->setAttribute(pmode);
    }
    if (state) {
        pmode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    } else {
        pmode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
    }
    _needsRedraw = true;
#else
    if (_stateManip.valid()) {
        _stateManip->setPolygonMode(state ? osg::PolygonMode::LINE : osg::PolygonMode::FILL);
        _needsRedraw = true;
    }
#endif
}

void Renderer::saveNamedViewpoint(const char *name)
{
    _viewpoints[name] = getViewpoint();
}

bool Renderer::removeNamedViewpoint(const char *name)
{
    ViewpointHashmap::iterator itr = _viewpoints.find(name);
    if (itr != _viewpoints.end()) {
        _viewpoints.erase(name);
        return true;
    } else {
        ERROR("Unknown viewpoint: '%s'", name);
        return false;
    }
}

bool Renderer::restoreNamedViewpoint(const char *name, double durationSecs)
{
    ViewpointHashmap::iterator itr = _viewpoints.find(name);
    if (itr != _viewpoints.end()) {
        setViewpoint(itr->second, durationSecs);
        return true;
    } else {
        ERROR("Unknown viewpoint: '%s'", name);
        return false;
    }
}

void Renderer::setViewpoint(const osgEarth::Viewpoint& v, double durationSecs)
{
    if (_manipulator.valid()) {
        TRACE("Setting viewpoint: %g %g %g %g %g %g",
              v.x(), v.y(), v.z(), v.getHeading(), v.getPitch(), v.getRange());
        _manipulator->setViewpoint(v, durationSecs);
        _needsRedraw = true;
    } else {
        ERROR("No manipulator");
    }
}

osgEarth::Viewpoint Renderer::getViewpoint()
{
    if (_manipulator.valid()) {
        return _manipulator->getViewpoint();
    } else {
        // Uninitialized, invalid viewpoint
        return osgEarth::Viewpoint();
    }
}

/**
 * \brief Map screen mouse coordinates to map coordinates
 *
 * This method assumes that mouse Y coordinates are 0 at the top
 * of the screen and increase going down if invertY is set, and 
 * 0 at the bottom and increase going up otherwise.
 */
bool Renderer::mapMouseCoords(float mouseX, float mouseY,
                              osgEarth::GeoPoint& map, bool invertY)
{
    if (!_mapNode.valid() || _mapNode->getTerrain() == NULL) {
        ERROR("No map");
        return false;
    }
    if (!_viewer.valid()) {
        ERROR("No Viewer");
        return false;
    }
    if (invertY) {
        mouseY = ((float)_windowHeight - mouseY);
    }
    osg::Vec3d world;
    if (_mapNode->getTerrain()->getWorldCoordsUnderMouse(_viewer->asView(), mouseX, mouseY, world)) {
        map.fromWorld(_mapNode->getMapSRS(), world);
        return true;
    }
    return false;
}

void Renderer::addImageLayer(const char *name,
                             osgEarth::TileSourceOptions& opts,
                             bool makeShared,
                             bool visible)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    TRACE("layer: %s", name);
    if (!opts.tileSize().isSet()) {
        opts.tileSize() = 256;
    }
    osgEarth::ImageLayerOptions layerOpts(name, opts);
    if (makeShared) {
        layerOpts.shared() = true;
    }
    if (!visible) {
        layerOpts.visible() = false;
    }
    _map->addImageLayer(new osgEarth::ImageLayer(layerOpts));
    _needsRedraw = true;
}

void Renderer::addColorFilter(const char *name,
                              const char *shader)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    if (layer == NULL) {
        TRACE("Image layer not found: %s", name);
        return;
    }
    osgEarth::Util::GLSLColorFilter *filter = new osgEarth::Util::GLSLColorFilter;
    filter->setCode(shader);
    //filter->setCode("color.rgb = color.r > 0.5 ? vec3(1.0) : vec3(0.0);");
    layer->addColorFilter(filter);
    _needsRedraw = true;
}

void Renderer::removeColorFilter(const char *name, int idx)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    if (layer == NULL) {
        TRACE("Image layer not found: %s", name);
        return;
    }
    if (idx < 0) {
        while (!layer->getColorFilters().empty()) {
            layer->removeColorFilter(layer->getColorFilters()[0]);
        }
    } else {
        layer->removeColorFilter(layer->getColorFilters().at(idx));
    }
    _needsRedraw = true;
}

void Renderer::removeImageLayer(const char *name)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    if (layer != NULL) {
        _map->removeImageLayer(layer);
        _needsRedraw = true;
    } else {
        TRACE("Image layer not found: %s", name);
    }
}

void Renderer::moveImageLayer(const char *name, unsigned int pos)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    if (layer != NULL) {
        _map->moveImageLayer(layer, pos);
        _needsRedraw = true;
    } else {
        TRACE("Image layer not found: %s", name);
    }
}

void Renderer::setImageLayerOpacity(const char *name, double opacity)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    if (layer != NULL) {
        layer->setOpacity(opacity);
        _needsRedraw = true;
    } else {
        TRACE("Image layer not found: %s", name);
    }
}

void Renderer::setImageLayerVisibility(const char *name, bool state)
{
#if OSGEARTH_MIN_VERSION_REQUIRED(2, 4, 0)
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ImageLayer *layer = _map->getImageLayerByName(name);
    if (layer != NULL) {
        layer->setVisible(state);
        _needsRedraw = true;
    } else {
        TRACE("Image layer not found: %s", name);
    }
#endif
}

void Renderer::addElevationLayer(const char *name,
                                 osgEarth::TileSourceOptions& opts)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    TRACE("layer: %s", name);
    if (!opts.tileSize().isSet()) {
        opts.tileSize() = 15;
    }
    osgEarth::ElevationLayerOptions layerOpts(name, opts);
    // XXX: GDAL does not report vertical datum, it should be specified here
    // Common options: geodetic (default), egm96, egm84, egm2008
    //layerOpts.verticalDatum() = "egm96";
    _map->addElevationLayer(new osgEarth::ElevationLayer(layerOpts));
    _needsRedraw = true;
}

void Renderer::removeElevationLayer(const char *name)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ElevationLayer *layer = _map->getElevationLayerByName(name);
    if (layer != NULL) {
        _map->removeElevationLayer(layer);
        _needsRedraw = true;
    } else {
        TRACE("Elevation layer not found: %s", name);
    }
}

void Renderer::moveElevationLayer(const char *name, unsigned int pos)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ElevationLayer *layer = _map->getElevationLayerByName(name);
    if (layer != NULL) {
        _map->moveElevationLayer(layer, pos);
        _needsRedraw = true;
    } else {
        TRACE("Elevation layer not found: %s", name);
    }
}

void Renderer::setElevationLayerVisibility(const char *name, bool state)
{
#if OSGEARTH_MIN_VERSION_REQUIRED(2, 4, 0)
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ElevationLayer *layer = _map->getElevationLayerByName(name);
    if (layer != NULL) {
        layer->setVisible(state);
        _needsRedraw = true;
    } else {
        TRACE("Elevation layer not found: %s", name);
    }
#endif
}

void Renderer::addModelLayer(const char *name, osgEarth::ModelSourceOptions& opts)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    TRACE("layer: %s", name);
    osgEarth::ModelLayerOptions layerOpts(name, opts);
    _map->addModelLayer(new osgEarth::ModelLayer(layerOpts));
    _needsRedraw = true;
}

void Renderer::removeModelLayer(const char *name)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ModelLayer *layer = _map->getModelLayerByName(name);
    if (layer != NULL) {
        _map->removeModelLayer(layer);
        _needsRedraw = true;
    } else {
        TRACE("Model layer not found: %s", name);
    }
}

void Renderer::moveModelLayer(const char *name, unsigned int pos)
{
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ModelLayer *layer = _map->getModelLayerByName(name);
    if (layer != NULL) {
        _map->moveModelLayer(layer, pos);
        _needsRedraw = true;
    } else {
        TRACE("Model layer not found: %s", name);
    }
}

void Renderer::setModelLayerOpacity(const char *name, double opacity)
{
#if OSGEARTH_MIN_VERSION_REQUIRED(2, 5, 0)
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ModelLayer *layer = _map->getModelLayerByName(name);
    if (layer != NULL) {
        layer->setOpacity(opacity);
        _needsRedraw = true;
    } else {
        TRACE("Model layer not found: %s", name);
    }
#endif
}

void Renderer::setModelLayerVisibility(const char *name, bool state)
{
#if OSGEARTH_MIN_VERSION_REQUIRED(2, 4, 0)
    if (!_map.valid()) {
        ERROR("No map");
        return;
    }
    osgEarth::ModelLayer *layer = _map->getModelLayerByName(name);
    if (layer != NULL) {
        layer->setVisible(state);
        _needsRedraw = true;
    } else {
        TRACE("Model layer not found: %s", name);
    }
#endif
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
    if (_viewer.valid()) {
#ifdef USE_OFFSCREEN_RENDERING
#ifdef USE_PBUFFER
        osg::ref_ptr<osg::GraphicsContext> pbuffer;
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->x = 0;
        traits->y = 0;
        traits->width = _windowWidth;
        traits->height = _windowHeight;
        traits->red = 8;
        traits->green = 8;
        traits->blue = 8;
        traits->alpha = 8;
        traits->windowDecoration = false;
        traits->pbuffer = true;
        traits->doubleBuffer = true;
        traits->sharedContext = 0;

        pbuffer = osg::GraphicsContext::createGraphicsContext(traits.get());
        if (pbuffer.valid()) {
            TRACE("Pixel buffer has been created successfully.");
        } else {
            ERROR("Pixel buffer has not been created successfully.");
        }
        osg::Camera *camera = new osg::Camera;
        camera->setGraphicsContext(pbuffer.get());
        camera->setViewport(new osg::Viewport(0, 0, _windowWidth, _windowHeight));
        GLenum buffer = pbuffer->getTraits()->doubleBuffer ? GL_BACK : GL_FRONT;
        camera->setDrawBuffer(buffer);
        camera->setReadBuffer(buffer);
        camera->setFinalDrawCallback(_captureCallback.get());
        _viewer->addSlave(camera, osg::Matrixd(), osg::Matrixd());
        _viewer->realize();
#else
        if (_captureCallback.valid()) {
            _captureCallback->getTexture()->setTextureSize(_windowWidth, _windowHeight);
        }
        osgViewer::ViewerBase::Windows windows;
        _viewer->getWindows(windows);
        if (windows.size() == 1) {
            windows[0]->setWindowRectangle(0, 0, _windowWidth, _windowHeight);
        } else {
            ERROR("Num windows: %lu", windows.size());
        }
#endif
#else
        osgViewer::ViewerBase::Windows windows;
        _viewer->getWindows(windows);
        if (windows.size() == 1) {
            windows[0]->setWindowRectangle(0, 0, _windowWidth, _windowHeight);
        } else {
            ERROR("Num windows: %lu", windows.size());
        }
#endif
        _needsRedraw = true;
    }
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
    if (_manipulator.valid()) {
        _manipulator->setRotation(osg::Quat(quat[1], quat[2], quat[3], quat[0]));
        _needsRedraw = true;
    }
}

/**
 * \brief Reset pan, zoom, clipping planes and optionally rotation
 *
 * \param[in] resetOrientation Reset the camera rotation/orientation also
 */
void Renderer::resetCamera(bool resetOrientation)
{
    TRACE("Enter: resetOrientation=%d", resetOrientation ? 1 : 0);
    if (_viewer.valid()) {
        _viewer->home();
        _needsRedraw = true;
    }
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
 */
void Renderer::panCamera(double x, double y)
{
    TRACE("Enter: %g %g", x, y);

    if (_manipulator.valid()) {
        // Wants mouse delta x,y in normalized screen coords
        _manipulator->pan(x, y);
        _needsRedraw = true;
    }
}

void Renderer::rotateCamera(double x, double y)
{
    TRACE("Enter: %g %g", x, y);

    if (_manipulator.valid()) {
        _manipulator->rotate(x, y);
        _needsRedraw = true;
    }
}

/**
 * \brief Dolly camera or set orthographic scaling based on camera type
 *
 * \param[in] y Mouse y coordinate in normalized screen coords
 */
void Renderer::zoomCamera(double y)
{
    TRACE("Enter: y: %g", y);

    if (_manipulator.valid()) {
        TRACE("camDist: %g", _manipulator->getDistance());
        // FIXME: zoom here wants y mouse coords in normalized viewport coords
#if 1
       _manipulator->zoom(0, y);
#else
        double dist = _manipulator->getDistance();
        dist *= (1.0 + y);
        _manipulator->setDistance(dist);
#endif
        _needsRedraw = true;
    }
}

/**
 * \brief Dolly camera to set distance from focal point
 *
 * \param[in] dist distance in map? coordinates
 */
void Renderer::setCameraDistance(double dist)
{
    TRACE("Enter: dist: %g", dist);

    if (_manipulator.valid()) {
        TRACE("camDist: %g", _manipulator->getDistance());

        _manipulator->setDistance(dist);

        _needsRedraw = true;
    }
}

void Renderer::keyPress(int key)
{
    osgGA::EventQueue *queue = getEventQueue();
    if (queue != NULL) {
        queue->keyPress(key);
        _needsRedraw = true;
    }
}

void Renderer::keyRelease(int key)
{
    osgGA::EventQueue *queue = getEventQueue();
    if (queue != NULL) {
        queue->keyRelease(key);
        _needsRedraw = true;
    }
}

void Renderer::setThrowingEnabled(bool state)
{
    if (_manipulator.valid()) {
        _manipulator->getSettings()->setThrowingEnabled(state);
    }
}

void Renderer::mouseDoubleClick(int button, double x, double y)
{
    osgGA::EventQueue *queue = getEventQueue();
    if (queue != NULL) {
        queue->mouseDoubleButtonPress((float)x, (float)y, button);
        _needsRedraw = true;
    }
}

void Renderer::mouseClick(int button, double x, double y)
{
    osgGA::EventQueue *queue = getEventQueue();
    if (queue != NULL) {
        queue->mouseButtonPress((float)x, (float)y, button);
        _needsRedraw = true;
    }
}

void Renderer::mouseDrag(int button, double x, double y)
{
    osgGA::EventQueue *queue = getEventQueue();
    if (queue != NULL) {
        queue->mouseMotion((float)x, (float)y);
        _needsRedraw = true;
    }
}

void Renderer::mouseRelease(int button, double x, double y)
{
    osgGA::EventQueue *queue = getEventQueue();
    if (queue != NULL) {
        queue->mouseButtonRelease((float)x, (float)y, button);
        _needsRedraw = true;
    }
}

void Renderer::mouseMotion(double x, double y)
{
    if (_mouseCoordsTool.valid()) {
#if 1
        osgGA::EventQueue *queue = getEventQueue();
        if (queue != NULL) {
            queue->mouseMotion((float)x, (float)y);
            _needsRedraw = true;
        }
#else
        if (_viewer.valid() && _coordsCallback.valid()) {
            osgEarth::GeoPoint mapPt;
            if (mapMouseCoords((float)x, (float)y, mapPt)) {
                _coordsCallback->set(mapPt, _viewer->asView(), _mapNode);
            } else {
                _coordsCallback->reset(_viewer->asView(), _mapNode);
            }
            _needsRedraw = true;
        }
#endif
    }
}

void Renderer::mouseScroll(int direction)
{
    osgGA::EventQueue *queue = getEventQueue();
    if (queue != NULL) {
        queue->mouseScroll((direction > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN));
        _needsRedraw = true;
    }
}

/**
 * \brief Set the RGB background color to render into the image
 */
void Renderer::setBackgroundColor(float color[3])
{
    _bgColor[0] = color[0];
    _bgColor[1] = color[1];
    _bgColor[2] = color[2];

    if (_viewer.valid()) {
        _viewer->getCamera()->setClearColor(osg::Vec4(color[0], color[1], color[2], 1));

        _needsRedraw = true;
    }
}

/**
 * \brief Sets flag to trigger rendering next time render() is called
 */
void Renderer::eventuallyRender()
{
    _needsRedraw = true;
}

/**
 * \brief Get a timeout in usecs for select()
 * 
 * If the paging thread is idle, returns <0 indicating that the
 * select call can block until data is available.  Otherwise,
 * if the frame render time was faster than the target frame 
 * rate, return the remaining frame time.
 */
long Renderer::getTimeout()
{
    if (!checkNeedToDoFrame())
        // <0 means no timeout, block until socket has data
        return -1L;
    if (_lastFrameTime < _minFrameTime) {
        return (long)1000000.0*(_minFrameTime - _lastFrameTime);
    } else {
        // No timeout (poll)
        return 0L;
    }
}

/**
 * \brief Check if paging thread is quiescent
 */
bool Renderer::isPagerIdle()
{
    if (!_viewer.valid())
        return true;
    else
        return (!_viewer->getDatabasePager()->requiresUpdateSceneGraph() &&
                !_viewer->getDatabasePager()->getRequestsInProgress());
}

/**
 * \brief Check is frame call is necessary to render and/or update
 * in response to events or timed actions
 */
bool Renderer::checkNeedToDoFrame()
{
    return (_needsRedraw ||
            (_viewer.valid() && _viewer->checkNeedToDoFrame()));
}

/**
 * \brief Cause the rendering to render a new image if needed
 *
 * The _needsRedraw flag indicates if a state change has occured since
 * the last rendered frame
 */
bool Renderer::render()
{
    if (_viewer.valid() && checkNeedToDoFrame()) {
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
    if (_captureCallback.valid())
        return _captureCallback->getImage();
    else
        return NULL;
}