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

#include "Types.h"
#include "Trace.h"

// Controls if TGA format is sent to client
//#define RENDER_TARGA
#define TARGA_BYTES_PER_PIXEL 3

namespace GeoVis {

/**
 * \brief GIS Renderer
 */
class Renderer
{
public:
    Renderer();
    virtual ~Renderer();

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

    void panCamera(double x, double y, bool absolute = true);

    void zoomCamera(double z, bool absolute = true);

    // Rendering an image

    void setBackgroundColor(float color[3]);

    void eventuallyRender();

    bool render();

    void getRenderedFrame(osg::Image *image);

private:
    void initCamera();

    bool _needsRedraw;
    int _windowWidth, _windowHeight;
    float _bgColor[3];

    osg::ref_ptr<osg::Node> _sceneRoot;
    //osgEarth::Map *_map;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
};

}

#endif
