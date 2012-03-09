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
    m_mvi_one_volume_param = cgGetNamedParameter(_cgFP, "modelViewInv");
    m_mv_one_volume_param = cgGetNamedParameter(_cgFP, "modelView");

    m_vol_one_volume_param = cgGetNamedParameter(_cgFP, "volume");
    m_tf_one_volume_param = cgGetNamedParameter(_cgFP, "tf");
    m_render_param_one_volume_param = cgGetNamedParameter(_cgFP, "renderParameters");
    m_option_one_volume_param = cgGetNamedParameter(_cgFP, "options");
}


