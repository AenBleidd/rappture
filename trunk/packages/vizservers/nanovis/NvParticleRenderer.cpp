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

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include <R2/R2FilePath.h>

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

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[0]);

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _psysTex[0]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#endif
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                              GL_TEXTURE_RECTANGLE_NV, _psysTex[0], 0);


    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[1]);

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _psysTex[1]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#endif
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                              GL_TEXTURE_RECTANGLE_NV, _psysTex[1], 0);
 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    glGenTextures(1, &_initPosTex);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _initPosTex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, NULL);
#endif

    CHECK_FRAMEBUFFER_STATUS();

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
                    p[4*index] = _slicePos;
                    p[4*index+1]= j/float(_psysHeight);
                    p[4*index+2]= i/float(_psysWidth);
                    break;
                case 1:
                    p[4*index]= j/float(_psysHeight);
                    p[4*index+1] = _slicePos;
                    p[4*index+2]= i/float(_psysWidth);
                    break;
                case 2:
                    p[4*index]= j/float(_psysHeight);
                    p[4*index+1]= i/float(_psysWidth);
                    p[4*index+2] = _slicePos;
                    break;
                default:
                    p[4*index] = 0;
                    p[4*index+1]= 0;
                    p[4*index+2]= 0;
                    p[4*index+3]= 0;
                }

                //shorter life span, quicker iterations
                p[4*index+3]= rand() / ((float) RAND_MAX) * 0.5  + 0.5f; 
            } else {
                p[4*index] = 0;
                p[4*index+1]= 0;
                p[4*index+2]= 0;
                p[4*index+3]= 0;
            }
        }
    }
}

void NvParticleRenderer::initialize()
{
    initializeDataArray();

    //also store the data on main memory for next initialization
    //memcpy(data, p, psys_width*psys_height*sizeof(Particle));

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _psysTex[0]);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);

    _flip = true;
    _reborn = false;

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _initPosTex);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);

    TRACE("init particles\n");
}

void NvParticleRenderer::reset()
{
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _psysTex[0]);
#ifdef HAVE_FLOAT_TEXTURES
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, 
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA,
                 _psysWidth, _psysHeight, 0, GL_RGBA, GL_FLOAT, (float*)_data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);
    _flip = true;
    _reborn = false;
    _psysFrame = 0;
}

void 
NvParticleRenderer::advect()
{
    if (_reborn) 
        reset();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    if (_flip) {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[1]);
        glEnable(GL_TEXTURE_RECTANGLE_NV);
        glBindTexture(GL_TEXTURE_RECTANGLE_NV, _psysTex[0]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, _psysWidth, _psysHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        //gluOrtho2D(0, _psysWidth, 0, _psysHeight);
        glOrtho(0, _psysWidth, 0, _psysHeight, -10.0f, 10.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        _advectionShader->bind(_psysTex[0], _initPosTex);

        draw_quad(_psysWidth, _psysHeight, _psysWidth, _psysHeight);

        glDisable(GL_TEXTURE_RECTANGLE_NV);
    } else {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _psysFbo[0]);
        glBindTexture(GL_TEXTURE_RECTANGLE_NV, _psysTex[1]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, _psysWidth, _psysHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        //gluOrtho2D(0, _psysWidth, 0, _psysHeight);
        glOrtho(0, _psysWidth, 0, _psysHeight, -10.0f, 10.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        _advectionShader->bind(_psysTex[1], _initPosTex);

        draw_quad(_psysWidth, _psysHeight, _psysWidth, _psysHeight);
    }

    _advectionShader->unbind();

    updateVertexBuffer();

    _flip = (!_flip);

    _psysFrame++;
    if (_psysFrame == _maxLife) {
        _psysFrame = 0;
        //        _reborn = true;
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void
NvParticleRenderer::updateVertexBuffer()
{
    _vertexArray->Read(_psysWidth, _psysHeight);

    //_vertexArray->LoadData(vert);     //does not work??
    //assert(glGetError()==0);
}

void
NvParticleRenderer::render()
{
    displayVertices();
}

void 
NvParticleRenderer::drawBoundingBox(float x0, float y0, float z0,
                                    float x1, float y1, float z1,
                                    float r, float g, float b, 
                                    float line_width)
{
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4d(r, g, b, 1.0);
    glLineWidth(line_width);
    
    glBegin(GL_LINE_LOOP); 
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y1, z0);
        glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z1);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x0, y1, z1);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x0, y0, z1);
        glVertex3d(x0, y1, z1);
        glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x1, y1, z0);
    }
    glEnd();

    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void
NvParticleRenderer::displayVertices()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_COLOR_MATERIAL);

    glPushMatrix();

    glTranslatef(_origin.x, _origin.y, _origin.z);
    glScaled(_scale.x, _scale.y, _scale.z);

    // TBD...
    /*
      drawBoundingBox(0, 0, 0, 
      1, 1, 1, 
      1, 1, 1, 2);

      drawBoundingBox(0, 0.5f / 4.5f, 0.5f / 4.5,
      1, 4.0f / 4.5f, 4.0f / 4.5,
      1, 0, 0, 2);

      drawBoundingBox(1/3.0f, 1.0f / 4.5f, 0.5f / 4.5,
      2/3.0f, 3.5f / 4.5f, 3.5f / 4.5,
      1, 1, 0, 2);
    */

    glPointSize(_particleSize);
    //glColor4f(.2,.2,.8,1.);
    glColor4f(_color.x, _color.y, _color.z, _color.w);
    glEnableClientState(GL_VERTEX_ARRAY);
    _vertexArray->SetPointer(0);
    glDrawArrays(GL_POINTS, 0, _psysWidth * _psysHeight);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

    glDisable(GL_DEPTH_TEST);

    //assert(glGetError()==0);
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
