/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_REGULAR_SHADER_H
#define NV_REGULAR_SHADER_H

#include "Volume.h"
#include "NvVolumeShader.h"

/// Shader for regular volume (uniform grid)
class NvRegularVolumeShader : public NvVolumeShader
{
public:
    NvRegularVolumeShader();

    virtual ~NvRegularVolumeShader();

    virtual void bind(unsigned int tfID, Volume *volume,
                      int sliceMode, float sampleRatio);

    virtual void unbind();

private:
    void init();
};

#endif
