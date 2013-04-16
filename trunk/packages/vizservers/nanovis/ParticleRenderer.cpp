/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* 
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  Authors:
 *    Wei Qiao <qiaow@purdue.edu>
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <GL/glew.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Color4f.h>

#include "ParticleRenderer.h"
#include "Volume.h"
#include "define.h"
#include "Trace.h"

using namespace vrmath;
using namespace nv;

ParticleRenderer::ParticleRenderer(int w, int h) :
    _initPosTex(0),
    _data(NULL),
    _psysFrame(0),
    _reborn(true),
    _flip(true),
    _maxLife(500),
    _particleSize(1.2),
    _vertexArray(NULL),
    _scale(1, 1, 1), 
    _origin(0, 0, 0),
    _activate(false),
    _slicePos(0.0),
    _sliceAxis(AXIS_Z),
    _color(0.2, 0.2, 1.0, 1.0),
    _psysWidth(w),
    _psysHeight(h)
{
    _data = new Particle[w * h];

    _vertexArray = new RenderVertexArray(_psysWidth * _psysHeight, 3, GL_FLOAT);

    assert(CheckGL(AT));

    glGenFramebuffersEXT(2, _psysFbo);
    glGenTextures(2, _psysTex);

    int fboOrig;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[0]);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _psysTex[0]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#endif
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                              GL_TEXTURE_RECTANGLE_ARB, _psysTex[0], 0);

    CHECK_FRAMEBUFFER_STATUS();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[1]);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _psysTex[1]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#endif
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                              GL_TEXTURE_RECTANGLE_ARB, _psysTex[1], 0);
 
    CHECK_FRAMEBUFFER_STATUS();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

    glGenTextures(1, &_initPosTex);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _initPosTex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#endif

    _advectionShader = new ParticleAdvectionShader();
}

ParticleRenderer::~ParticleRenderer()
{
    glDeleteTextures(1, &_initPosTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[0]);
    glDeleteTextures(1, _psysTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[1]);
    glDeleteTextures(1, _psysTex+1);

    glDeleteFramebuffersEXT(2, _psysFbo);

    delete _advectionShader;
    delete _vertexArray;
    delete [] _data;
}

void
ParticleRenderer::initializeDataArray()
{
    TRACE("Enter axis: %d pos: %g", _sliceAxis, _slicePos);

    memset(_data, 0, sizeof(Particle) * _psysWidth * _psysHeight);
 
    bool hasParticle;
    for (int i = 0; i < _psysWidth; i++) {
        for (int j = 0; j < _psysHeight; j++) {
            Particle *p = &_data[i + _psysHeight*j];
            hasParticle = (rand() % 256) > 150;
             if (hasParticle) {
                //assign any location (x,y,z) in range [0,1]
                switch (_sliceAxis) {
                case AXIS_X:
                    p->x = _slicePos;
                    p->y = j/float(_psysHeight);
                    p->z = i/float(_psysWidth);
                    break;
                case AXIS_Y:
                    p->x = j/float(_psysHeight);
                    p->y = _slicePos;
                    p->z = i/float(_psysWidth);
                    break;
                case AXIS_Z:
                    p->x = j/float(_psysHeight);
                    p->y = i/float(_psysWidth);
                    p->z = _slicePos;
                    break;
                default:
                    ERROR("Unknown axis");
                }
                //shorter life span, quicker iterations
                p->life = rand() / ((float) RAND_MAX) * 0.5  + 0.5f; 
            }
        }
    }
}

void
ParticleRenderer::initialize()
{
    TRACE("Enter");

    initializeDataArray();

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _psysTex[0]);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

    _flip = true;
    _reborn = false;
    _psysFrame = 0;

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _initPosTex);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

    TRACE("Leave");
}

void
ParticleRenderer::reset()
{
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _psysTex[0]);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    _flip = true;
    _reborn = false;
    _psysFrame = 0;
}

void
ParticleRenderer::advect()
{
    TRACE("Enter");

    if (_reborn) 
        reset();

    glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    int fboOrig;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    int fbo, tex;
    if (_flip) {
        fbo = 1;
        tex = 0;
    } else {
        fbo = 0;
        tex = 1;
    }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[fbo]);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _psysTex[tex]);

    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, _psysWidth, _psysHeight);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, _psysWidth, 0, _psysHeight, -10.0f, 10.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    _advectionShader->bind(_psysTex[tex], _initPosTex, _psysFrame == 0);

    draw_quad(_psysWidth, _psysHeight, _psysWidth, _psysHeight);

    _advectionShader->unbind();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glDisable(GL_TEXTURE_RECTANGLE_ARB);

    updateVertexBuffer();

    _flip = (!_flip);

    _psysFrame++;
    if (_psysFrame == _maxLife) {
        //_psysFrame = 0;
        //_reborn = true;
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

    glPopAttrib();
}

void
ParticleRenderer::updateVertexBuffer()
{
    _vertexArray->read(_psysWidth, _psysHeight);
}

void 
ParticleRenderer::render()
{
    if (_psysFrame == 0) {
        TRACE("Initializing vertex array");
        advect();
    }

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_COLOR_MATERIAL);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glTranslatef(_origin.x, _origin.y, _origin.z);
    glScaled(_scale.x, _scale.y, _scale.z);

    glPointSize(_particleSize);
    glColor4f(_color.x, _color.y, _color.z, _color.w);

    glEnableClientState(GL_VERTEX_ARRAY);
    _vertexArray->setPointer(0);
    glDrawArrays(GL_POINTS, 0, _psysWidth * _psysHeight);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
    glPopAttrib();
}

void 
ParticleRenderer::setVectorField(Volume *volume)
{
    Vector3f bmin, bmax;
    volume->getBounds(bmin, bmax);
    _origin = bmin;
    Vector3f scale(bmax.x-bmin.x, bmax.y-bmin.y, bmax.z-bmin.z);
    _scale.set(scale.x, scale.y, scale.z);
    _advectionShader->setScale(_scale);
    _advectionShader->setVelocityVolume(volume->textureID(),
                                        volume->wAxis.max());
}

void
ParticleRenderer::setAxis(FlowSliceAxis axis)
{
    if (axis != _sliceAxis) {
        _sliceAxis = axis;
        initialize();
    }
}

void
ParticleRenderer::setPos(float pos)
{
    if (pos != _slicePos) {
        _slicePos = pos;
        initialize();
    }
}
