#include <R2/R2FilePath.h>
#include <Trace.h>
#include "NvParticleAdvectionShader.h"

NvParticleAdvectionShader::NvParticleAdvectionShader()
: _scale(1.0f, 1.0f, 1.0f), _velocityVolumeID(0), _max(1.0f), _timeStep(0.005f)
{
    init();
}

NvParticleAdvectionShader::~NvParticleAdvectionShader()
{
}

void NvParticleAdvectionShader::init()
{
    R2string path = R2FilePath::getInstance()->getPath("update_pos.cg");
    
    if (path.getLength() == 0)
    {
        Trace("%s is not found\n", (const char*) path);
    }

    _cgFP= cgCreateProgramFromFile(g_context, CG_SOURCE, path,
        CG_PROFILE_FP30, "main", NULL);


    cgGLLoadProgram(_cgFP);

    _posTimestepParam  = cgGetNamedParameter(_cgFP, "timestep");
    _maxParam  = cgGetNamedParameter(_cgFP, "max");
    _velTexParam = cgGetNamedParameter(_cgFP, "vel_tex");
    _posTexParam = cgGetNamedParameter(_cgFP, "pos_tex");
    _scaleParam = cgGetNamedParameter(_cgFP, "scale");
}

