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

    void getEyeSpaceBounds(const Mat4x4& mv,
                           double& xMin, double& xMax,
                           double& yMin, double& yMax,
                           double& zNear, double& zFar);

    VolumeInterpolator *_volumeInterpolator;

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
};

#endif
