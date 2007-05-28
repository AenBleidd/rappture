#include "NvRegularVolumeShader.h"

NvRegularVolumeShader::NvRegularVolumeShader()
{
    init();
}

NvRegularVolumeShader::~NvRegularVolumeShader()
{
}

void NvRegularVolumeShader::init()
{
    _cgFP= cgCreateProgramFromFile(g_context, CG_SOURCE,
                "/opt/nanovis/lib/shaders/one_volume.cg", CG_PROFILE_FP30, "main", NULL);
    cgGLLoadProgram(_cgFP);

    m_mvi_one_volume_param = cgGetNamedParameter(_cgFP, "modelViewInv");
    m_mv_one_volume_param = cgGetNamedParameter(_cgFP, "modelView");

    m_vol_one_volume_param = cgGetNamedParameter(_cgFP, "volume");
    m_tf_one_volume_param = cgGetNamedParameter(_cgFP, "tf");
    m_render_param_one_volume_param = cgGetNamedParameter(_cgFP, "renderParameters");
}


