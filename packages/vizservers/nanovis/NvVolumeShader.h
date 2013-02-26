/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_VOLUME_SHADER_H
#define NV_VOLUME_SHADER_H

#include "NvShader.h"

class Volume;

class NvVolumeShader : public NvShader
{
public :
    virtual ~NvVolumeShader();

    virtual void bind(unsigned int tfID, Volume *volume,
                      int sliceMode, float sampleRatio) = 0;

    virtual void unbind()
    {
        NvShader::unbind();
    }

protected :
    NvVolumeShader();
};


#endif // 

