/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * PlaneRenderer.h : PlaneRenderer class for 2D visualization
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef PLANE_RENDERER_H
#define PLANE_RENDERER_H

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>

#include <vector>

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "global.h"
#include "TransferFunction.h"
#include "Texture2D.h"

class PlaneRenderer
{
public:
    PlaneRenderer(CGcontext _context, int width, int height);

    ~PlaneRenderer();

    int add_plane(Texture2D* _p, TransferFunction* _tf);

    // Add a plane and its transfer function. We require a transfer function
    // when a plane is added.
    void remove_plane(int index);

    void set_active_plane(int index); //set the active plane to be rendered

    void set_screen_size(int w, int h);	//change the rendering size

    void render();

private:
    void init_shaders();

    void activate_shader(int plane_index);

    void deactivate_shader();

    std::vector<Texture2D *> _plane;	// Array of volumes
    std::vector<TransferFunction *> _tf; // Array of corresponding transfer functions 
    int _active_plane;		// The active plane, only one is rendered
    int _n_planes;

    int _render_width;   //render size
    int _render_height;  

    //cg related
    CGcontext _g_context;	// The Nvidia cg context 
    CGprogram _fprog;
    CGparameter _data_param;
    CGparameter _tf_param;
    CGparameter _render_param;
};

#endif
