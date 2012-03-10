/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "NvRegularVolumeShader.h"
#include "global.h"

NvRegularVolumeShader::NvRegularVolumeShader()
{
    init();
}

NvRegularVolumeShader::~NvRegularVolumeShader()
{
}

void NvRegularVolumeShader::init()
{
    _cgFP = LoadCgSourceProgram(g_context, "one_volume.cg", CG_PROFILE_FP30, 
                                "main");
    _mvi_one_volume_param = cgGetNamedParameter(_cgFP, "modelViewInv");
    _mv_one_volume_param = cgGetNamedParameter(_cgFP, "modelView");

    _vol_one_volume_param = cgGetNamedParameter(_cgFP, "volume");
    _tf_one_volume_param = cgGetNamedParameter(_cgFP, "tf");
    _render_param_one_volume_param = cgGetNamedParameter(_cgFP, "renderParameters");
    _option_one_volume_param = cgGetNamedParameter(_cgFP, "options");
}


