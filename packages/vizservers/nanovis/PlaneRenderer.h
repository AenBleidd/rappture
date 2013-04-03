/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  Authors:
 *    Wei Qiao <qiaow@purdue.edu>
 */
#ifndef PLANE_RENDERER_H
#define PLANE_RENDERER_H

#include <vector>

#include "ColorTableShader.h"
#include "TransferFunction.h"
#include "Texture2D.h"

namespace nv {

class PlaneRenderer
{
public:
    PlaneRenderer(int width, int height);

    ~PlaneRenderer();

    /// Add a plane and its transfer function.
    int addPlane(Texture2D *p, TransferFunction *tf);

    void removePlane(int index);

    /// Set the active plane to be rendered
    void setActivePlane(int index);

    /// Change the rendering size
    void setScreenSize(int w, int h);

    void render();

private:
    std::vector<Texture2D *> _plane;	 ///< Array of images
    std::vector<TransferFunction *> _tf; ///< Array of corresponding transfer functions 
    int _activePlane;                    ///< The active plane, only one is rendered
    int _numPlanes;

    int _renderWidth;
    int _renderHeight;  

    ColorTableShader *_shader;
};

}

#endif
