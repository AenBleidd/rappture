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
    _cgFP= cgCreateProgramFromFile(g_context, CG_SOURCE,
        "/opt/nanovis/lib/shaders/update_pos.cg", CG_PROFILE_FP30, "main", NULL);

    cgGLLoadProgram(_cgFP);

    _posTimestepParam  = cgGetNamedParameter(_cgFP, "timestep");
    _velTexParam = cgGetNamedParameter(_cgFP, "vel_tex");
    _posTexParam = cgGetNamedParameter(_cgFP, "pos_tex");
    _scaleParam = cgGetNamedParameter(_cgFP, "scale");
    // TBD..
    //cgGLSetTextureParameter(_velTexParam, volume);
    cgGLSetParameter3f(_scaleParam, scale.x, scale.y, scale.z);
}

