/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_VOLUME_RENDERER_H
#define NV_VOLUME_RENDERER_H

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>
#include <vrmath/Matrix4x4d.h>

#include "Volume.h"
#include "VolumeInterpolator.h"
#include "RegularVolumeShader.h"
#include "ZincBlendeVolumeShader.h"
#include "StdVertexShader.h"

namespace nv {

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

private:
    void initShaders();

    void activateVolumeShader(Volume *vol, 
                              vrmath::Vector4f& objPlaneS,
                              vrmath::Vector4f& objPlaneT,
                              vrmath::Vector4f& objPlaneR,
                              bool sliceMode, float sampleRatio);

    void deactivateVolumeShader();

    static void drawBoundingBox(float x0, float y0, float z0,
                                float x1, float y1, float z1,
                                float r, float g, float b, float line_width);

    static void getEyeSpaceBounds(const vrmath::Vector3f& worldMin,
                                  const vrmath::Vector3f& worldMax,
                                  const vrmath::Matrix4x4d& mv,
                                  vrmath::Vector3f& eyeMin,
                                  vrmath::Vector3f& eyeMax);

    VolumeInterpolator *_volumeInterpolator;

    /**
     * Shader for single slice cutplane render
     */
    Shader *_cutplaneShader;

    /**
     * Shader for rendering a single cubic volume
     */
    RegularVolumeShader *_regularVolumeShader;

    /**
     * Shader for rendering a single zincblende orbital.  A
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
    ZincBlendeVolumeShader *_zincBlendeShader;

    /**
     * standard vertex shader 
     */
    StdVertexShader *_stdVertexShader;
};

}

#endif
