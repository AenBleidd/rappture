/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvLIC.h: line integral convolution class
 *
 * ======================================================================
 *  AUTHOR:  Insoo Woo <iwoo@purdue.edu, Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <R2/R2FilePath.h>

#include "NvLIC.h"
#include "NvShader.h"
#include "define.h"
#include "Trace.h"

#define NPN 256   //resolution of background pattern
#define DM ((float) (1.0/(NMESH-1.0))) //distance in world coords between mesh lines
#define SCALE 3.0 //scale for background pattern. small value -> fine texture

NvLIC::NvLIC(int size, int width, int height, int axis, 
             const Vector3& offset) :
    Renderable(Vector3(0.0f, 0.0f, 0.0f)),
    _width(width),
    _height(height),
    _size(size),
    _scale(1.0f, 1.0f, 1.0f),
    _origin(0, 0, 0),
    _offset(offset),
    _axis(axis),
    _iframe(0),
    _Npat(64),
    _alpha((int)0.12*255),
    _tmax(NPIX/(SCALE*NPN)),
    _dmax(SCALE/NPIX),
    _max(1.0f),
    _disListID(0),
    _vectorFieldId(0),
    _activate(false)
{
    _sliceVector = new float[_size*_size*4];
    memset(_sliceVector, 0, sizeof(float) * _size * _size * 4);

    int fboOrig;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

    //initialize the pattern texture
    glGenTextures(1, &_patternTex);
    glBindTexture(GL_TEXTURE_2D, _patternTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NPN, NPN, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    //initialize frame buffer objects
    //render buffer for projecting 3D velocity onto a 2D plane
    glGenFramebuffersEXT(1, &_velFbo);
    glGenTextures(1, &_sliceVectorTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _velFbo);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _sliceVectorTex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, _size, _size, 
                 0, GL_RGBA, GL_FLOAT, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                              GL_TEXTURE_RECTANGLE_ARB, _sliceVectorTex, 0);

    //render buffer for the convolution
    glGenFramebuffersEXT(1, &_fbo);
    glGenTextures(1, &_colorTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

    //initialize color texture for lic
    glBindTexture(GL_TEXTURE_2D, _colorTex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, _width, _height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, _colorTex, 0);

    // Check framebuffer completeness at the end of initialization.
    CHECK_FRAMEBUFFER_STATUS();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

    _renderVelShader = new NvShader();
    _renderVelShader->loadFragmentProgram("render_vel.cg", "main");

    _velTexParamRenderVel =
        _renderVelShader->getNamedParameterFromFP("vel_tex");

    _planeNormalParamRenderVel =
        _renderVelShader->getNamedParameterFromFP("plane_normal");

    _maxParam = 
        _renderVelShader->getNamedParameterFromFP("vmax");

    makePatterns();
}

NvLIC::~NvLIC()
{
    glDeleteTextures(1, &_patternTex);
    glDeleteTextures(1, &_magTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _velFbo);
    glDeleteTextures(1, &_sliceVectorTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
    glDeleteTextures(1, &_colorTex);

    GLuint buffers[2] = {_velFbo, _fbo};
    glDeleteFramebuffersEXT(2, buffers);

    glDeleteLists(_disListID, _Npat);

    delete _renderVelShader;

    delete [] _sliceVector;
}

void 
NvLIC::makePatterns() 
{ 
    TRACE("begin makePatterns\n");
    if (_disListID > 0) {
        glDeleteLists(_disListID, _Npat);
    }
    _disListID = glGenLists(_Npat);

    TRACE("DisplayList : %d\n", _disListID);

    int lut[256];
    int phase[NPN][NPN];
    GLubyte pat[NPN][NPN][4];
    int i, j, k, t;

    for (i = 0; i < 256; i++) {
        lut[i] = i < 127 ? 0 : 255;
    }
    for (i = 0; i < NPN; i++) {
        for (j = 0; j < NPN; j++) {
            phase[i][j] = rand() >> 8;
        }
    }
    for (k = 0; k < _Npat; k++) {
        t = (k << 8) / _Npat;
        for (i = 0; i < NPN; i++) {
            for (j = 0; j < NPN; j++) {
                pat[i][j][0] = pat[i][j][1] = pat[i][j][2] = 
                    lut[(t + phase[i][j]) % 255];
                pat[i][j][3] = _alpha;
            }
        }
        glNewList(_disListID + k, GL_COMPILE);
        glBindTexture(GL_TEXTURE_2D, _patternTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NPN, NPN, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pat);
        glEndList();
    }

    glBindTexture(GL_TEXTURE_2D, _patternTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NPN, NPN, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pat);
    TRACE("finish makePatterns\n");
}

void NvLIC::makeMagnitudes()
{
    GLubyte mag[NMESH][NMESH][4];

    //read vector field
    for (int i = 0; i < NMESH; i++) {
        for (int j = 0; j < NMESH; j++) {
            float x = DM*i;
            float y = DM*j;
 
            float magnitude = sqrt(x*x+y*y)/1.414;

            //from green to red
            GLubyte r = (GLubyte)floor(magnitude*255);
            GLubyte g = 0;
            GLubyte b = 255 - r;
            GLubyte a = 122;

            mag[i][j][0] = r;
            mag[i][j][1] = g;
            mag[i][j][2] = b;
            mag[i][j][3] = a;
        }
    }
    glGenTextures(1, &_magTex);
    glBindTexture(GL_TEXTURE_2D, _magTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NMESH, NMESH, 0, GL_RGBA, 
                 GL_UNSIGNED_BYTE, mag);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void 
NvLIC::getSlice()
{
    int fboOrig;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _velFbo);

    glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, _size, _size);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, _size, 0, _size);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, _vectorFieldId);

    cgGLSetTextureParameter(_velTexParamRenderVel, _vectorFieldId);
    cgGLEnableTextureParameter(_velTexParamRenderVel);
    cgGLSetParameter4f(_planeNormalParamRenderVel, 1., 1., 0., 0);
    cgGLSetParameter1f(_maxParam, _max);

    _renderVelShader->bind();

    glBegin(GL_QUADS);
    {
        switch (_axis) {
        case 0 :
            // TBD..
            glTexCoord3f(_offset.x, 0., 0.); glVertex2f(0.,    0.);
            glTexCoord3f(_offset.x, 1., 0.); glVertex2f(_size, 0.);
            glTexCoord3f(_offset.x, 1., 1.); glVertex2f(_size, _size);
            glTexCoord3f(_offset.x, 0., 1.); glVertex2f(0.,    _size);
                break;
        case 1 :
            // TBD..
            glTexCoord3f(0., _offset.y, 0.); glVertex2f(0.,    0.);
            glTexCoord3f(1., _offset.y, 0.); glVertex2f(_size, 0.);
            glTexCoord3f(1., _offset.y, 1.); glVertex2f(_size, _size);
            glTexCoord3f(0., _offset.y, 1.); glVertex2f(0.,    _size);
                break;
        case 2 :
            glTexCoord3f(0., 0., _offset.z); glVertex2f(0.,    0.);
            glTexCoord3f(1., 0., _offset.z); glVertex2f(_size, 0.);
            glTexCoord3f(1., 1., _offset.z); glVertex2f(_size, _size);
            glTexCoord3f(0., 1., _offset.z); glVertex2f(0.,    _size);
            break;
        }
    }
    glEnd();

    _renderVelShader->unbind();

    cgGLDisableTextureParameter(_velTexParamRenderVel);

    glBindTexture(GL_TEXTURE_3D, 0);
    glDisable(GL_TEXTURE_3D);

    //read the vectors
    glReadPixels(0, 0, _size, _size, GL_RGBA, GL_FLOAT, _sliceVector);

    int lim = _size * _size * 4;
    float *v = _sliceVector;
    for (int i = 0; i < lim; ++i) {
        if (isnan(*v)) {
            *v = 0.0f;    
        }
        ++v;
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);
}

//line integral convolution
void 
NvLIC::convolve()
{
    if (_vectorFieldId == 0) {
        return;
    }

    int   i, j; 
    float x1, x2, y, px, py;

    int fboOrig;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

    glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glViewport(0, 0, (GLsizei) NPIX, (GLsizei) NPIX);
    //glTranslatef(-1.0, -1.0, 0.0); 
    //glScalef(2.0, 2.0, 1.0);
    glOrtho(0.0f, 1.0f, 0.0f, 1.0f, -10.0f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT);
    //glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);

    //_sa = 0.010*cos(_iframe*2.0*M_PI/200.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _patternTex);
    _sa = 0.01;
 
    glColor4f(1, 1, 1, 1);
    for (i = 0; i < NMESH-1; i++) {
        x1 = DM*i; x2 = x1 + DM;
        glBegin(GL_QUAD_STRIP);
        for (j = 0; j < NMESH-1; j++) {
            y = DM*j;
            glTexCoord2f(x1, y); 
            getVelocity(x1, y, &px, &py);
            glVertex2f(px, py);

            glTexCoord2f(x2, y); 
            getVelocity(x2, y, &px, &py); 
            glVertex2f(px, py);
        }
        glEnd();
    }
    _iframe = _iframe + 1;

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glCallList(_iframe % _Npat + _disListID);
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(0.0,   0.0);   glVertex2f(0.0, 0.0);
        glTexCoord2f(0.0,   _tmax); glVertex2f(0.0, 1.0);
        glTexCoord2f(_tmax, 0.0);   glVertex2f(1.0, 0.0);
        glTexCoord2f(_tmax, _tmax); glVertex2f(1.0, 1.0);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);

    /*
    //inject dye
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.,0.8,0.,1.);
    glBegin(GL_QUADS);
    glVertex2d(0.6, 0.6);
    glVertex2d(0.6, 0.62);
    glVertex2d(0.62, 0.62);
    glVertex2d(0.62, 0.6);
    glEnd();
    */

    glDisable(GL_BLEND);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, NPIX, NPIX, 0);

    /*
    //blend magnitude texture
    glBindTexture(GL_TEXTURE_2D, _magTex);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
    glTexCoord2f(0.0,  1.0); glVertex2f(0.0, 1.);
    glTexCoord2f(1.0, 1.0);  glVertex2f(1., 1.);
    glTexCoord2f(1.0, 0.0); glVertex2f(1., 0.0);
    glEnd();
    */

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);
}

void 
NvLIC::render()
{
    if (_vectorFieldId == 0) {
        return;
    }

    //draw line integral convolution quad

    glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glBindTexture(GL_TEXTURE_2D, _colorTex);
    glEnable(GL_TEXTURE_2D);
    //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    //glEnable(GL_LIGHTING);
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    float w = 1.0f / _scale.x;
    glTranslatef(_origin.x, _origin.y, _origin.z);
    glScalef(1.0f, 1.0f / _scale.y / w, 1.0f / _scale.z / w);

    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    switch (_axis) {
    case 0:
        glNormal3f(1, 0, 0);
        glTexCoord2f(0, 0); glVertex3f(_offset.x, 0, 0);
        glTexCoord2f(1, 0); glVertex3f(_offset.x, 1, 0);
        glTexCoord2f(1, 1); glVertex3f(_offset.x, 1, 1);
        glTexCoord2f(0, 1); glVertex3f(_offset.x, 0, 1);
        break;
    case 1:
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0); glVertex3f(0, _offset.y, 0);
        glTexCoord2f(1, 0); glVertex3f(1, _offset.y, 0);
        glTexCoord2f(1, 1); glVertex3f(1, _offset.y, 1);
        glTexCoord2f(0, 1); glVertex3f(0, _offset.y, 1);
        break;
    case 2:
        glNormal3f(0, 0, 1);
        glTexCoord2f(0, 0); glVertex3f(0, 0, _offset.z);
        glTexCoord2f(1, 0); glVertex3f(1, 0, _offset.z);
        glTexCoord2f(1, 1); glVertex3f(1, 1, _offset.z);
        glTexCoord2f(0, 1); glVertex3f(0, 1, _offset.z);
        break;
    }
    glEnd();

    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);

    glPopAttrib();
}

void 
NvLIC::setVectorField(unsigned int texID, const Vector3& origin, 
                      float scaleX, float scaleY, float scaleZ, float max)
{
    TRACE("NvLIC: vector field is assigned [%d]\n", texID);
    _vectorFieldId = texID;
    _origin = origin;
    _scale = Vector3(scaleX, scaleY, scaleZ);
    _max = max;

    makePatterns();

    getSlice();
}

void 
NvLIC::getVelocity(float x, float y, float *px, float *py) 
{
   float vx, vy, r;

   int xi = (int)(x*_size);
   int yi = (int)(y*_size);

    //TRACE("(xi yi) = (%d %d), ", xi, yi);
   vx = _sliceVector[4 * (xi+yi*_size)];
   vy = _sliceVector[4 * (xi+yi*_size)+1];
   r  = vx*vx + vy*vy;

    //TRACE("(vx vx) = (%f %f), r=%f, ", vx, vy, r);
   if (r > (_dmax * _dmax)) {
      r  = sqrt(r);
      vx *= _dmax/r;
      vy *= _dmax/r;
   }

   *px = x + vx;
   *py = y + vy;

    //TRACE("vel %f %f -> %f %f, (dmax = %f)\n", x, y, *px, *py, dmax);
}

void 
NvLIC::setOffset(float v)
{
    switch (_axis) {
    case 0 : _offset.x = v; break;
    case 1 : _offset.y = v; break;
    case 2 : _offset.z = v; break;
    }
    getSlice();
}

void NvLIC::setAxis(int axis)
{
    _axis = axis;
}

void NvLIC::reset()
{
    makePatterns();
}
