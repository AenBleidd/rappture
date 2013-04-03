/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * PlaneRenderer.h : PlaneRenderer class for 2D visualization
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef PLANE_RENDERER_H
#define PLANE_RENDERER_H

#include <vector>

#include "NvColorTableShader.h"
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

    NvColorTableShader *_shader;
};

}

#endif
