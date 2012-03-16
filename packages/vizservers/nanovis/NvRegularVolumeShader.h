/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_REGULAR_SHADER_H
#define NV_REGULAR_SHADER_H

#include "Volume.h"
#include "NvVolumeShader.h"

class NvRegularVolumeShader : public NvVolumeShader
{
public:
    NvRegularVolumeShader();
    ~NvRegularVolumeShader();

    void bind(unsigned int tfID, Volume* volume, int sliceMode);
    void unbind();

private:
    void init();

    CGparameter _vol_one_volume_param;
    CGparameter _tf_one_volume_param;
    CGparameter _mvi_one_volume_param;
    CGparameter _mv_one_volume_param;
    CGparameter _render_param_one_volume_param;
    CGparameter _option_one_volume_param;
};

#endif
