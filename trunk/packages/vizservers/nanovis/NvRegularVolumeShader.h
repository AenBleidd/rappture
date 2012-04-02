/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_REGULAR_SHADER_H
#define NV_REGULAR_SHADER_H

#include "Volume.h"
#include "NvVolumeShader.h"

class NvRegularVolumeShader : public NvVolumeShader
{
public:
    NvRegularVolumeShader();
    virtual ~NvRegularVolumeShader();

    virtual void bind(unsigned int tfID, Volume* volume, int sliceMode);
    virtual void unbind();

private:
    void init();

    CGparameter _volOneVolumeParam;
    CGparameter _tfOneVolumeParam;
    CGparameter _mviOneVolumeParam;
    CGparameter _mvOneVolumeParam;
    CGparameter _renderParamOneVolumeParam;
    CGparameter _optionOneVolumeParam;
};

#endif
