#include <R2/R2FilePath.h>
#include "NvParticleAdvectionShader.h"

NvParticleAdvectionShader::NvParticleAdvectionShader(const Vector3& scale)
{
    init(scale);
}

NvParticleAdvectionShader::~NvParticleAdvectionShader()
{
}

void NvParticleAdvectionShader::init(const Vector3& scale)
{
    R2string path = R2FilePath::getInstance()->getPath("update_pos.cg");
    _cgFP= cgCreateProgramFromFile(g_context, CG_SOURCE, path,
        CG_PROFILE_FP30, "main", NULL);

    cgGLLoadProgram(_cgFP);

    _posTimestepParam  = cgGetNamedParameter(_cgFP, "timestep");
    _velTexParam = cgGetNamedParameter(_cgFP, "vel_tex");
    _posTexParam = cgGetNamedParameter(_cgFP, "pos_tex");
    _scaleParam = cgGetNamedParameter(_cgFP, "scale");
    // TBD..
    //cgGLSetTextureParameter(_velTexParam, volume);
    cgGLSetParameter3f(_scaleParam, scale.x, scale.y, scale.z);
}

