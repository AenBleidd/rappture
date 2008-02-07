#include <R2/R2FilePath.h>
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
    R2string path = R2FilePath::getInstance()->getPath("one_volume.cg");
    _cgFP= cgCreateProgramFromFile(g_context, CG_SOURCE, path, CG_PROFILE_FP30, "main", NULL);
    cgGLLoadProgram(_cgFP);

    m_mvi_one_volume_param = cgGetNamedParameter(_cgFP, "modelViewInv");
    m_mv_one_volume_param = cgGetNamedParameter(_cgFP, "modelView");

    m_vol_one_volume_param = cgGetNamedParameter(_cgFP, "volume");
    m_tf_one_volume_param = cgGetNamedParameter(_cgFP, "tf");
    m_render_param_one_volume_param = cgGetNamedParameter(_cgFP, "renderParameters");
    m_option_one_volume_param = cgGetNamedParameter(_cgFP, "options");
}


