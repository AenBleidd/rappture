/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * PlaneRenderer.cpp : PlaneRenderer class for volume visualization
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

#include "global.h"
#include "PlaneRenderer.h"
#include "Trace.h"

PlaneRenderer::PlaneRenderer(CGcontext _context, int _width, int _height) :
    _active_plane(-1),
    _n_planes(0),
    _render_width(_width),
    _render_height(_height),
    _g_context(_context)
{
    _plane.clear();
    _tf.clear();
    init_shaders();
}

PlaneRenderer::~PlaneRenderer() 
{
}

//initialize the render shader
void 
PlaneRenderer::init_shaders()
{
    //plane rendering shader
    _fprog = LoadCgSourceProgram(_g_context, "one_plane.cg", CG_PROFILE_FP30, 
                                 NULL);
    _data_param = cgGetNamedParameter(_fprog, "data");
    _tf_param = cgGetNamedParameter(_fprog, "tf");
    _render_param = cgGetNamedParameter(_fprog, "render_param");
}

int 
PlaneRenderer::add_plane(Texture2D* _p, TransferFunction* tfPtr)
{
    int ret = _n_planes;

    _plane.push_back(_p);
    _tf.push_back(tfPtr);

    if (ret == 0)
        _active_plane = ret;

    _n_planes++;
    return ret;
}

void
PlaneRenderer::remove_plane(int index) {
    std::vector<Texture2D*>::iterator piter = _plane.begin()+index;
    std::vector<TransferFunction*>::iterator tfiter = _tf.begin()+index;

    _plane.erase(piter);
    _tf.erase(tfiter);

    _n_planes--;
}

void 
PlaneRenderer::render()
{
    if (_n_planes == 0)
        return;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glViewport(0, 0, _render_width, _render_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, _render_width, 0, _render_height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //glColor3f(1., 1., 1.);         //MUST HAVE THIS LINE!!!

    //if no active plane
    if (_active_plane == -1)
        return;

    activate_shader(_active_plane);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(_render_width, 0);
    glTexCoord2f(1, 1); glVertex2f(_render_width, _render_height);
    glTexCoord2f(0, 1); glVertex2f(0, _render_height);
    glEnd();
    deactivate_shader();

}

void 
PlaneRenderer::activate_shader(int index)
{
    cgGLSetTextureParameter(_data_param, _plane[index]->id);
    cgGLSetTextureParameter(_tf_param, _tf[index]->id());
    cgGLEnableTextureParameter(_data_param);
    cgGLEnableTextureParameter(_tf_param);

    cgGLSetParameter4f(_render_param, 0., 0., 0., 0.);

    cgGLBindProgram(_fprog);
    cgGLEnableProfile(CG_PROFILE_FP30);
}

void 
PlaneRenderer::deactivate_shader()
{
    cgGLDisableProfile(CG_PROFILE_FP30);
    cgGLDisableTextureParameter(_data_param);
    cgGLDisableTextureParameter(_tf_param);
}

void
PlaneRenderer::set_active_plane(int index) 
{
    _active_plane = index;
}

void
PlaneRenderer::set_screen_size(int w, int h) 
{
    _render_width = w;
    _render_height = h;
}
