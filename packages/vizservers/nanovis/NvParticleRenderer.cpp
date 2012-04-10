/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvParticleRenderer.cpp: particle system class
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <GL/glew.h>

#include "NvParticleRenderer.h"
#include "define.h"
#include "Trace.h"

NvParticleAdvectionShader *NvParticleRenderer::_advectionShader = NULL;

class NvParticleAdvectionShaderInstance
{
public :
    NvParticleAdvectionShaderInstance()
    {}

    ~NvParticleAdvectionShaderInstance()
    {
        if (NvParticleRenderer::_advectionShader) {
            delete NvParticleRenderer::_advectionShader;
        }
    }
};

NvParticleAdvectionShaderInstance shaderInstance;

NvParticleRenderer::NvParticleRenderer(int w, int h) :
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
    _sliceAxis(0),
    _color(0.2, 0.2, 1.0, 1.0),
    _psysWidth(w),
    _psysHeight(h)
{
    _data = new Particle[w * h];
    memset(_data, 0, sizeof(Particle) * w * h);

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

    if (_advectionShader == NULL) {
        _advectionShader = new NvParticleAdvectionShader();
    }
}

NvParticleRenderer::~NvParticleRenderer()
{
    glDeleteTextures(1, &_initPosTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[0]);
    glDeleteTextures(1, _psysTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[1]);
    glDeleteTextures(1, _psysTex+1);

    glDeleteFramebuffersEXT(2, _psysFbo);

    delete _vertexArray;
    delete [] _data;
}

void NvParticleRenderer::initializeDataArray()
{
    size_t n = _psysWidth * _psysHeight * 4;
    memset(_data, 0, sizeof(float)* n);

    int index;
    bool particle;
    float *p = (float *)_data;
    for (int i = 0; i < _psysWidth; i++) {
        for (int j = 0; j < _psysHeight; j++) {
            index = i + _psysHeight*j;
            particle = (rand() % 256) > 150;
            if (particle) {
                //assign any location (x,y,z) in range [0,1]
                switch (_sliceAxis) {
                case 0:
                    p[4*index]   = _slicePos;
                    p[4*index+1] = j/float(_psysHeight);
                    p[4*index+2] = i/float(_psysWidth);
                    break;
                case 1:
                    p[4*index]   = j/float(_psysHeight);
                    p[4*index+1] = _slicePos;
                    p[4*index+2] = i/float(_psysWidth);
                    break;
                case 2:
                    p[4*index]   = j/float(_psysHeight);
                    p[4*index+1] = i/float(_psysWidth);
                    p[4*index+2] = _slicePos;
                    break;
                default:
                    p[4*index]   = 0;
                    p[4*index+1] = 0;
                    p[4*index+2] = 0;
                    p[4*index+3] = 0;
                }

                //shorter life span, quicker iterations
                p[4*index+3] = rand() / ((float) RAND_MAX) * 0.5  + 0.5f; 
            } else {
                p[4*index]   = 0;
                p[4*index+1] = 0;
                p[4*index+2] = 0;
                p[4*index+3] = 0;
            }
        }
    }
}

void NvParticleRenderer::initialize()
{
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

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _initPosTex);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

    TRACE("init particles\n");
}

void NvParticleRenderer::reset()
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
NvParticleRenderer::advect()
{
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
    //gluOrtho2D(0, _psysWidth, 0, _psysHeight);
    glOrtho(0, _psysWidth, 0, _psysHeight, -10.0f, 10.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    _advectionShader->bind(_psysTex[tex], _initPosTex);

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
        _psysFrame = 0;
        // _reborn = true;
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

    glPopAttrib();
}

void
NvParticleRenderer::updateVertexBuffer()
{
    _vertexArray->read(_psysWidth, _psysHeight);

    //_vertexArray->loadData(vert);     //does not work??
    //assert(glGetError()==0);
}

void
NvParticleRenderer::render()
{
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
NvParticleRenderer::setVectorField(unsigned int texID, const Vector3& origin, 
                                   float scaleX, float scaleY, float scaleZ, 
                                   float max)
{
    _origin = origin;
    _scale.set(scaleX, scaleY, scaleZ);
    _advectionShader->setScale(_scale);
    _advectionShader->setVelocityVolume(texID, max);
}

void NvParticleRenderer::setAxis(int axis)
{
    _sliceAxis = axis;
    initializeDataArray();
}

void NvParticleRenderer::setPos(float pos)
{
    _slicePos = pos;
    initializeDataArray();
}
