#include <R2/R2FilePath.h>
#include <Trace.h>
#include "NvParticleAdvectionShader.h"
#include <global.h>

NvParticleAdvectionShader::NvParticleAdvectionShader() : 
    _velocityVolumeID(0), 
    _scale(1.0f, 1.0f, 1.0f), 
    _max(1.0f), 
    _timeStep(0.005f)
{
    init();
}

NvParticleAdvectionShader::~NvParticleAdvectionShader()
{
}

void NvParticleAdvectionShader::init()
{
    _cgFP = LoadCgSourceProgram(g_context, "update_pos.cg", CG_PROFILE_FP30, 
	"main");
    _posTimestepParam  = cgGetNamedParameter(_cgFP, "timestep");
    _maxParam  = cgGetNamedParameter(_cgFP, "max");
    _velTexParam = cgGetNamedParameter(_cgFP, "vel_tex");
    _posTexParam = cgGetNamedParameter(_cgFP, "pos_tex");
    _scaleParam = cgGetNamedParameter(_cgFP, "scale");
}

