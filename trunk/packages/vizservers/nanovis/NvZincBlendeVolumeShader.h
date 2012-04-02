/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_ZINCBLENDE_SHADER_H
#define NV_ZINCBLENDE_SHADER_H

#include "ZincBlendeVolume.h"
#include "NvVolumeShader.h"

class NvZincBlendeVolumeShader : public NvVolumeShader
{
public:
    NvZincBlendeVolumeShader();

    virtual ~NvZincBlendeVolumeShader();

    virtual void bind(unsigned int tfID, Volume *volume, int sliceMode);

    virtual void unbind();

private:
    void init();

    CGparameter _tfParam;
    CGparameter _volumeAParam;
    CGparameter _volumeBParam;
    CGparameter _cellSizeParam;
    CGparameter _mviParam;
    CGparameter _renderParam;
    CGparameter _optionOneVolumeParam;
};

#endif 
