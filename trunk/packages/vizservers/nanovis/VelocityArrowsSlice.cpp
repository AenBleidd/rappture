/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "nvconf.h"

#include <math.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <R2/R2FilePath.h>
#include <Image.h>
#include <ImageLoaderFactory.h>
#include <ImageLoader.h>

#include "nanovis.h"
#include "VelocityArrowsSlice.h"
#include "NvShader.h"
#include "NvCamera.h"

static inline float deg2rad(float deg)
{
    return ((deg * M_PI) / 180.);
}

static inline float rad2deg(float rad)
{
    return ((rad * 180.) / M_PI);
}

VelocityArrowsSlice::VelocityArrowsSlice() :
    _vectorFieldGraphicsID(0),
    _vfXscale(0),
    _vfYscale(0),
    _vfZscale(0),
    _slicePos(0.5f),
    _axis(2),
    _fbo(0),
    _tex(0),
    _maxPointSize(1.0f),
    _renderTargetWidth(128),
    _renderTargetHeight(128),
    _velocities(NULL),
    _projectionVector(1, 1, 0),
    _tickCountForMinSizeAxis(10),
    _tickCountX(0),
    _tickCountY(0),
    _tickCountZ(0),
    _pointCount(0),
    _maxVelocityScale(1, 1, 1),
    _arrowColor(1, 1, 0),
    _enabled(false),
    _dirty(true),
    _vertexBufferGraphicsID(0),
    _arrowsTex(NULL),
    _renderMode(LINES)
{
    axis(2);

    _queryVelocityFP.loadFragmentProgram("queryvelocity.cg", "main");

    _particleShader.loadVertexProgram("velocityslicevp.cg", "vpmain");
    _particleShader.loadFragmentProgram("velocityslicefp.cg", "fpmain");

    createRenderTarget();

    std::string path = R2FilePath::getInstance()->getPath("arrows.bmp");
    if (!path.empty()) {
        ImageLoader *loader = ImageLoaderFactory::getInstance()->createLoader("bmp");
        if (loader != NULL) {
            Image *image = loader->load(path.c_str(), Image::IMG_RGBA);
            if (image != NULL) {
                unsigned char *bytes = (unsigned char *)image->getImageBuffer();
                if (bytes != NULL) {
                    _arrowsTex = new Texture2D(image->getWidth(), image->getHeight(),
                                               GL_UNSIGNED_BYTE, GL_LINEAR, 4, NULL);
                    _arrowsTex->setWrapS(GL_MIRRORED_REPEAT);
                    _arrowsTex->setWrapT(GL_MIRRORED_REPEAT);
                    _arrowsTex->initialize(image->getImageBuffer());
                }
                delete image;
            } else {
                ERROR("Failed to load arrows image\n");
            }
            delete loader;
        } else {
            ERROR("Couldn't find loader for arrows image\n");
        }
    } else {
        ERROR("Couldn't find arrows image\n");
    }

    GLfloat minMax[2];
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, minMax);
    TRACE("Aliased point size range: %g %g\n", minMax[0], minMax[1]);
    _maxPointSize = minMax[1];
    glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, minMax);
    TRACE("Smooth point size range: %g %g\n", minMax[0], minMax[1]);
    _maxPointSize = minMax[1] > _maxPointSize ? minMax[1] : _maxPointSize;
    TRACE("Max point size: %g\n", _maxPointSize);

    TRACE("Leaving VelocityArrowsSlice constructor\n");
}

VelocityArrowsSlice::~VelocityArrowsSlice()
{
    if (_tex != 0) {
        glDeleteTextures(1, &_tex);
    }
    if (_fbo != 0) {
        glDeleteFramebuffersEXT(1, &_fbo);
    }
    if (_arrowsTex != NULL) {
        delete _arrowsTex;
    }
    if (_vertexBufferGraphicsID != 0) {
        glDeleteBuffers(1, &_vertexBufferGraphicsID);
    }
    if (_velocities != NULL) {
        delete [] _velocities;
    }
}

void VelocityArrowsSlice::createRenderTarget()
{
    if (_velocities != NULL) {
        delete [] _velocities;
    }
    _velocities = new Vector3[_renderTargetWidth * _renderTargetHeight];

    if (_vertexBufferGraphicsID != 0) {
        glDeleteBuffers(1, &_vertexBufferGraphicsID);
    }

    glGenBuffers(1, &_vertexBufferGraphicsID);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, _vertexBufferGraphicsID);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, _renderTargetWidth * _renderTargetHeight * 3 * sizeof(float), 
                    0, GL_DYNAMIC_DRAW_ARB);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    if (_tex != 0) {
        glDeleteTextures(1, &_tex);
    }
    if (_fbo != 0) {
        glDeleteFramebuffersEXT(1, &_fbo);
    }

    glGenFramebuffersEXT(1, &_fbo);
    glGenTextures(1, &_tex);
    int fboOrig;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _tex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
                 _renderTargetWidth, _renderTargetHeight, 0,
                 GL_RGBA, GL_FLOAT, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                              GL_TEXTURE_RECTANGLE_ARB, _tex, 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);
}

void VelocityArrowsSlice::axis(int axis)
{
    _axis = axis;
    switch (_axis) {
    case 0:
        _projectionVector.x = 0;
        _projectionVector.y = 1;
        _projectionVector.z = 1;
        break;
    case 1 :
        _projectionVector.x = 1;
        _projectionVector.y = 0;
        _projectionVector.z = 1;
        break;
    case 2:
        _projectionVector.x = 1;
        _projectionVector.y = 1;
        _projectionVector.z = 0;
        break;
    }
    _dirty = true;
}

void VelocityArrowsSlice::queryVelocity()
{
    if (!_enabled) return;

    glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    int fboOrig;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

    _queryVelocityFP.bind();
    _queryVelocityFP.setFPTextureParameter("vfield", _vectorFieldGraphicsID);

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, _renderTargetWidth, _renderTargetHeight);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, _renderTargetWidth, 0, _renderTargetHeight, -10.0f, 10.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    switch (_axis) {
    case 0:
        glTexCoord3f(_slicePos, 0, 0); glVertex2i(0,                  0);
        glTexCoord3f(_slicePos, 1, 0); glVertex2i(_renderTargetWidth, 0);
        glTexCoord3f(_slicePos, 1, 1); glVertex2i(_renderTargetWidth, _renderTargetHeight);
        glTexCoord3f(_slicePos, 0, 1); glVertex2i(0,                  _renderTargetHeight);
        break;
    case 1:
        glTexCoord3f(0, _slicePos, 0); glVertex2i(0,                  0);
        glTexCoord3f(1, _slicePos, 0); glVertex2i(_renderTargetWidth, 0);
        glTexCoord3f(1, _slicePos, 1); glVertex2i(_renderTargetWidth, _renderTargetHeight);
        glTexCoord3f(0, _slicePos, 1); glVertex2i(0,                  _renderTargetHeight);
        break;
    case 2:
    default:
        glTexCoord3f(0, 0, _slicePos); glVertex2i(0,                  0);
        glTexCoord3f(1, 0, _slicePos); glVertex2i(_renderTargetWidth, 0);
        glTexCoord3f(1, 1, _slicePos); glVertex2i(_renderTargetWidth, _renderTargetHeight);
        glTexCoord3f(0, 1, _slicePos); glVertex2i(0,                  _renderTargetHeight);
        break;
    }
    glEnd();

    _queryVelocityFP.disableFPTextureParameter("vfield");
    _queryVelocityFP.unbind();

    glReadPixels(0, 0, _renderTargetWidth, _renderTargetHeight, GL_RGB, GL_FLOAT, _velocities);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

    glPopAttrib();
}

static void drawLineArrow(int axis)
{
    glBegin(GL_LINES);
    switch (axis) {
    case 0: // ZY plane
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.0, 0.0, 1.0);

        glVertex3f(0.0, 0.0, 1.0);
        glVertex3f(0.0, 0.25, 0.75);

        glVertex3f(0.0, 0.0, 1.0);
        glVertex3f(0.0, -0.25, 0.75);
        break;
    case 1: // XZ plane
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(1.0, 0.0, 0.0);

        glVertex3f(1.0, 0.0, 0.0);
        glVertex3f(0.75, 0.0, 0.25);

        glVertex3f(1.0, 0.0, 0.0);
        glVertex3f(0.75, 0.0, -0.25);
        break;
    case 2: // XY plane
    default:
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(1.0, 0.0, 0.0);

        glVertex3f(1.0, 0.0, 0.0);
        glVertex3f(0.75, 0.25, 0.0);

        glVertex3f(1.0, 0.0, 0.0);
        glVertex3f(0.75, -0.25, 0.0);
        break;
    }
    glEnd();
}

void VelocityArrowsSlice::render()
{
    if (!_enabled)
        return;

    if (_dirty) {
        computeSamplingTicks();
        queryVelocity();
        _dirty = false;
    }

    TRACE("_vf: %g %g %g\n", _vfXscale,_vfYscale, _vfZscale);
    TRACE("_maxVelocityScale: %g %g %g\n",
          _maxVelocityScale.x, _maxVelocityScale.y, _maxVelocityScale.z);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glScalef(_vfXscale,_vfYscale, _vfZscale);
    glTranslatef(-0.5f, -0.5f, -0.5f);

    if (_renderMode == LINES) {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glLineWidth(2.0);
        glColor3f(_arrowColor.x, _arrowColor.y, _arrowColor.z);
 
        Vector3 pos;
        Vector3 vel;
        Vector3 refVec;
        Vector3 blue(0, 0, 1);
        Vector3 red(1, 0, 0);

        int index = 0, icount, jcount;
        switch (_axis) {
        case 0:
            icount = _tickCountZ;
            jcount = _tickCountY;
            refVec.set(0, 0, 1);
            break;
        case 1:
            icount = _tickCountZ;
            jcount = _tickCountX;
            refVec.set(1, 0, 0);
            break;
        case 2:
        default:
            icount = _tickCountY;
            jcount = _tickCountX;
            refVec.set(1, 0, 0);
            break;
        }

        for (int i = 0; i < icount; ++i) {
            for (int j = 0; j < jcount; ++j, ++index) {
                pos = _samplingPositions[index];
                // Normalized velocity: [-1,1] components
                // Project 3D vector onto sample plane
                vel = _velocities[index].scale(_projectionVector);
                // Length: [0,1]
                double length = vel.length();
                if (length < 0.0 || length > 1.0) {
                    TRACE("***vec: (%g, %g, %g) length: %g***\n", vel.x, vel.y, vel.z, length);
                    continue;
                }
                if (length > 1.0e-6) {
                    Vector3 vnorm = vel.normalize();
                    Vector3 rotationAxis = refVec.cross(vnorm);
                    double angle = rad2deg(acos(refVec.dot(vnorm)));
                    Vector3 color = blue * (1.0 - length) + red * length;
                    float scale = length;
                    if (scale < 0.10) scale = 0.10;
                    glMatrixMode(GL_MODELVIEW);
                    glPushMatrix();
                    glColor3f(color.x, color.y, color.z);
                    glTranslatef(pos.x, pos.y, pos.z);
                    glScalef(2.0 * _maxVelocityScale.x,
                             2.0 * _maxVelocityScale.y,
                             2.0 * _maxVelocityScale.z);
                    glScalef(scale, scale, scale);
                    if (angle > 1.0e-6 || angle < -1.0e-6)
                        glRotated(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);
                    drawLineArrow(_axis);
                    glPopMatrix();
                }
            }
        }

        glLineWidth(1.0);
    } else {
        glColor4f(_arrowColor.x, _arrowColor.y, _arrowColor.z, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
#if 0
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
#else
        glDisable(GL_BLEND);
#endif
        glAlphaFunc(GL_GREATER, 0.6);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_POINT_SPRITE_ARB);
        glPointSize(20);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

        _arrowsTex->activate();
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        GLfloat atten[] = {1, 0, 0};
        glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, atten);
        glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 0.0f);
        glPointParameterfARB(GL_POINT_SIZE_MIN_ARB, 1.0f);
        glPointParameterfARB(GL_POINT_SIZE_MAX_ARB, _maxPointSize);
        glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);

        _particleShader.bind();
        _particleShader.setVPTextureParameter("vfield", _vectorFieldGraphicsID);
        _particleShader.setFPTextureParameter("arrows", _arrowsTex->id());
        _particleShader.setVPParameter1f("tanHalfFOV",
                                         tan(NanoVis::getCamera()->fov() * 0.5) * NanoVis::winHeight * 0.5);
        _particleShader.setGLStateMatrixVPParameter("modelview",
                                                    NvShader::MODELVIEW_MATRIX,
                                                    NvShader::MATRIX_IDENTITY);
        _particleShader.setGLStateMatrixVPParameter("mvp",
                                                    NvShader::MODELVIEW_PROJECTION_MATRIX,
                                                    NvShader::MATRIX_IDENTITY);

        glEnableClientState(GL_VERTEX_ARRAY);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, _vertexBufferGraphicsID);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        //glEnableClientState(GL_COLOR_ARRAY);

        // TBD..
        glDrawArrays(GL_POINTS, 0, _pointCount);
        glPointSize(1);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
        glDisable(GL_POINT_SPRITE_ARB);

        _particleShader.disableVPTextureParameter("vfield");
        _particleShader.disableFPTextureParameter("arrows");
        _particleShader.unbind();

        glActiveTexture(GL_TEXTURE0);
        glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_FALSE);
        _arrowsTex->deactivate();
    }

    glPopMatrix();
    glPopAttrib();
}

void 
VelocityArrowsSlice::setVectorField(unsigned int vfGraphicsID, const Vector3& origin,
                                    float xScale, float yScale, float zScale, float max)
{
    _vectorFieldGraphicsID = vfGraphicsID;
    _vfXscale = xScale;
    _vfYscale = yScale;
    _vfZscale = zScale;

    _dirty = true;
}

void VelocityArrowsSlice::computeSamplingTicks()
{
    if (_vfXscale < _vfYscale) {
        if (_vfXscale < _vfZscale || _vfZscale == 0.0) {
            // vfXscale
            _tickCountX = _tickCountForMinSizeAxis;

            float step = _vfXscale / (_tickCountX + 1);

            _tickCountY = (int)(_vfYscale/step);
            _tickCountZ = (int)(_vfZscale/step);
        } else {
            // vfZscale
            _tickCountZ = _tickCountForMinSizeAxis;

            float step = _vfZscale / (_tickCountZ + 1);
            _tickCountX = (int)(_vfXscale/step);
            _tickCountY = (int)(_vfYscale/step);
        }
    } else {
        if (_vfYscale < _vfZscale || _vfZscale == 0.0) {
            // _vfYscale
            _tickCountY = _tickCountForMinSizeAxis;

            float step = _vfYscale / (_tickCountY + 1);
            _tickCountX = (int)(_vfXscale/step);
            _tickCountZ = (int)(_vfZscale/step);
        } else {
            // vfZscale
            _tickCountZ = _tickCountForMinSizeAxis;

            float step = _vfZscale / (_tickCountZ + 1);
            _tickCountX = (int)(_vfXscale/step);
            _tickCountY = (int)(_vfYscale/step);
        }
    }

    switch (_axis) {
    case 0:
        _tickCountX = 1;
        _renderTargetWidth = _tickCountY;
        _renderTargetHeight = _tickCountZ;
        break;
    case 1:
        _tickCountY = 1;
        _renderTargetWidth = _tickCountX;
        _renderTargetHeight = _tickCountZ;
        break;
    default:
    case 2:
        _tickCountZ = 1;
        _renderTargetWidth = _tickCountX;
        _renderTargetHeight = _tickCountY;
        break;
    }

    _maxVelocityScale.x = (1.0f / _tickCountX) * 0.8f;
    _maxVelocityScale.y = (1.0f / _tickCountY) * 0.8f;
    _maxVelocityScale.z = (1.0f / _tickCountZ) * 0.8f;

    TRACE("Tick counts: %d %d %d\n", _tickCountX, _tickCountY, _tickCountZ);

    int pointCount = _tickCountX * _tickCountY * _tickCountZ;
    if (_pointCount != pointCount) {
        _samplingPositions.clear();
        _samplingPositions.reserve(pointCount);
        _pointCount = pointCount;
    }

    createRenderTarget();

    Vector3 pos;
    Vector3 *pinfo = NULL;
    if (_renderMode == GLYPHS) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, _vertexBufferGraphicsID);
        pinfo = (Vector3 *)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    }

    if (_axis == 2) {
        for (int y = 1; y <= _tickCountY; ++y) {
            for (int x = 1; x <= _tickCountX; ++x) {
                pos.x = (1.0f / (_tickCountX + 1)) * x;
                pos.y = (1.0f / (_tickCountY + 1)) * y;
                pos.z = _slicePos;
                if (_renderMode == LINES) {
                    _samplingPositions.push_back(pos);
                } else {
                    *pinfo = pos;
                    ++pinfo;
                }
            }
        }
    } else if (_axis == 1) {
        for (int z = 1; z <= _tickCountZ; ++z) {
            for (int x = 1; x <= _tickCountX; ++x) {
                pos.x = (1.0f / (_tickCountX + 1)) * x;
                pos.y = _slicePos;
                pos.z = (1.0f / (_tickCountZ + 1)) * z;
                if (_renderMode == LINES) {
                    _samplingPositions.push_back(pos);
                } else {
                    *pinfo = pos;
                    ++pinfo;
                }
            }
        }
    } else if (_axis == 0) {
        for (int z = 1; z <= _tickCountZ; ++z) {
            for (int y = 1; y <= _tickCountY; ++y) {
                pos.x = _slicePos;
                pos.y = (1.0f / (_tickCountY + 1)) * y;
                pos.z = (1.0f / (_tickCountZ + 1)) * z;
                if (_renderMode == LINES) {
                    _samplingPositions.push_back(pos);
                } else {
                    *pinfo = pos;
                    ++pinfo;
                }
            }
        }
    }

    if (_renderMode == GLYPHS) {
        glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
    }
}
