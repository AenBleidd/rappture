/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>

#include "global.h"
#include "NvShader.h"

CGcontext g_context = 0;

void NvInitCG()
{
    g_context = cgCreateContext();
}

NvShader::NvShader():
    _cgVP(0),
    _cgFP(0)
{
}

NvShader::~NvShader()
{
    resetPrograms();
}

void NvShader::loadVertexProgram(const char *fileName, const char *entryPoint)
{
    resetPrograms();

    _cgVP = LoadCgSourceProgram(g_context, fileName, CG_PROFILE_VP30, entryPoint);
}

void NvShader::loadFragmentProgram(const char *fileName, const char *entryPoint)
{
    _cgFP = LoadCgSourceProgram(g_context, fileName, CG_PROFILE_FP30, entryPoint);
}

void NvShader::resetPrograms()
{
    if (_cgVP > 0) {
        cgDestroyProgram(_cgVP);
    }

    if (_cgFP > 0) {
        cgDestroyProgram(_cgFP);
    }
}

void NvShader::setErrorCallback(NvCgCallbackFunction callback)
{
    TRACE("NvShader callback\n");
    cgSetErrorCallback(callback);
}
