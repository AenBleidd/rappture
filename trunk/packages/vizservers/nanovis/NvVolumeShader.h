/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __NV_VOLUME_SHADER_H__
#define __NV_VOLUME_SHADER_H__

#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include "NvShader.h"

class Volume;

class NvVolumeShader : public NvShader {
protected :
    NvVolumeShader();

public :
    virtual ~NvVolumeShader();

public :
    virtual  void bind(unsigned int tfID, Volume* volume, int sliceMode) = 0;
    virtual void unbind() = 0;
};


#endif // 

