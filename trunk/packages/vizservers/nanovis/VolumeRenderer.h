/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * VolumeRenderer.h : VolumeRenderer class for volume visualization
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
#ifndef _VOLUME_RENDERER_H_
#define _VOLUME_RENDERER_H_

#include <GL/glew.h>

#include "Mat4x4.h"
#include "Volume.h"
#include "VolumeInterpolator.h"
#include "NvRegularVolumeShader.h"
#include "NvZincBlendeVolumeShader.h"
#include "NvStdVertexShader.h"

class VolumeRenderer
{
public:
    VolumeRenderer();

    ~VolumeRenderer();

    /// render all enabled volumes
    void renderAll();

    void specular(float val);

    void diffuse(float val);

    /// control independently
    void setSliceMode(bool val)
    {
        _sliceMode = val;
    }

    void setVolumeMode(bool val)
    {
        _volumeMode = val;
    }

    /// switch_cutplane_mode
    void switchSliceMode()
    {
        _sliceMode = (!_sliceMode);
    }

    void switchVolumeMode()
    {
        _volumeMode = (!_volumeMode);
    }

    void clearAnimatedVolumeInfo()
    {
        _volumeInterpolator->clearAll();
    }

    void addAnimatedVolume(Volume* volume)
    {
        _volumeInterpolator->addVolume(volume);
    }

    void startVolumeAnimation()
    {
        _volumeInterpolator->start();
    }

    void stopVolumeAnimation()
    {
        _volumeInterpolator->stop();
    }

    VolumeInterpolator* getVolumeInterpolator()
    {
        return _volumeInterpolator;
    }

    friend class NanoVis;

private:
    void initShaders();

    void activateVolumeShader(Volume *vol, bool slice_mode);

    void deactivateVolumeShader();

    void drawBoundingBox(float x0, float y0, float z0,
                         float x1, float y1, float z1,
                         float r, float g, float b, float line_width);

    void getNearFarZ(const Mat4x4& mv, double& zNear, double& zFar);

    bool initFont(const char *filename);

    /// there are two sets of font in the texture. 0, 1
    void glPrint(char *string, int set);

    /// draw label using bitmap texture
    void drawLabel(Volume *vol);

    /// Register the location of each alphabet in
    void buildFont();

    VolumeInterpolator *_volumeInterpolator;

    bool _sliceMode;  ///< enable cut planes
    bool _volumeMode; ///< enable full volume rendering

    /** 
     * shader parameters for rendering a single cubic volume
     */
    NvRegularVolumeShader *_regularVolumeShader;

    /**
     * Shader parameters for rendering a single zincblende orbital.  A
     * simulation contains S, P, D and SS, total of 4 orbitals. A full
     * rendering requires 4 zincblende orbital volumes.  A zincblende orbital
     * volume is decomposed into two "interlocking" cubic volumes and passed
     * to the shader.  We render each orbital with its own independent
     * transfer functions then blend the result.
     * 
     * The engine is already capable of rendering multiple volumes and combine
     * them. Thus, we just invoke this shader on S, P, D and SS orbitals with
     * different transfor functions. The result is a multi-orbital rendering.
     * This is equivalent to rendering 4 unrelated data sets occupying the
     * same space.
     */
    NvZincBlendeVolumeShader *_zincBlendeShader;

    /**
     * standard vertex shader 
     */
    NvStdVertexShader *_stdVertexShader;

    GLuint _fontBase;      ///< The base of the font display list.
    GLuint _fontTexture;   ///< The id of the font texture
};

#endif
