/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  Authors:
 *    Wei Qiao <qiaow@purdue.edu>
 */

#include <GL/glew.h>

#include "PlaneRenderer.h"
#include "Trace.h"

using namespace nv;

PlaneRenderer::PlaneRenderer(int width, int height) :
    _activePlane(-1),
    _numPlanes(0),
    _renderWidth(width),
    _renderHeight(height),
    _shader(new ColorTableShader())
{
    _plane.clear();
    _tf.clear();
}

PlaneRenderer::~PlaneRenderer() 
{
    delete _shader;
}

int 
PlaneRenderer::addPlane(Texture2D *planeTexture, TransferFunction *tf)
{
    int ret = _numPlanes;

    _plane.push_back(planeTexture);
    _tf.push_back(tf);

    if (ret == 0)
        _activePlane = ret;

    _numPlanes++;
    return ret;
}

void
PlaneRenderer::removePlane(int index)
{
    std::vector<Texture2D *>::iterator piter = _plane.begin()+index;
    std::vector<TransferFunction *>::iterator tfiter = _tf.begin()+index;

    _plane.erase(piter);
    _tf.erase(tfiter);

    _numPlanes--;
}

void 
PlaneRenderer::render()
{
    if (_numPlanes == 0)
        return;

    glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glViewport(0, 0, _renderWidth, _renderHeight);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, _renderWidth, 0, _renderHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    //glColor3f(1., 1., 1.);         //MUST HAVE THIS LINE!!!

    //if no active plane
    if (_activePlane == -1)
        return;

    _shader->bind(_plane[_activePlane], _tf[_activePlane]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(_renderWidth, 0);
    glTexCoord2f(1, 1); glVertex2f(_renderWidth, _renderHeight);
    glTexCoord2f(0, 1); glVertex2f(0, _renderHeight);
    glEnd();
    _shader->unbind();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

void
PlaneRenderer::setActivePlane(int index) 
{
    _activePlane = index;
}

void
PlaneRenderer::setScreenSize(int w, int h) 
{
    _renderWidth = w;
    _renderHeight = h;
}
