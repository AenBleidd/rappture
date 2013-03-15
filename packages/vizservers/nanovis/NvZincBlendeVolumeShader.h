/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_ZINCBLENDE_SHADER_H
#define NV_ZINCBLENDE_SHADER_H

#include "ZincBlendeVolume.h"
#include "NvVolumeShader.h"

class NvZincBlendeVolumeShader : public NvVolumeShader
{
public:
    NvZincBlendeVolumeShader();

    virtual ~NvZincBlendeVolumeShader();

    virtual void bind(unsigned int tfID, Volume *volume,
                      int sliceMode, float sampleRatio);

    virtual void unbind();

private:
    void init();
};

#endif 
