#include "NvShader.h"
#include <stdio.h>

CGcontext g_context = 0;

void NvInitCG()
{
    g_context = cgCreateContext();
}

NvShader::NvShader()
: _cgVP(0), _cgFP(0)
{
}

NvShader::~NvShader()
{
}

void NvShader::loadVertexProgram(const char* fileName, const char* entryPoint)
{
    resetPrograms();

    printf("[%s] loading\n", fileName);
    _cgVP = cgCreateProgramFromFile(g_context, CG_SOURCE, fileName, CG_PROFILE_VP30,
                            entryPoint, NULL);

    cgGLLoadProgram(_cgVP);
}

void NvShader::loadFragmentProgram(const char* fileName, const char* entryPoint)
{
    printf("[%s] loading\n", fileName);
    _cgFP = cgCreateProgramFromFile(g_context, CG_SOURCE, fileName, CG_PROFILE_FP30,
                            entryPoint, NULL);

    cgGLLoadProgram(_cgFP);
}

void NvShader::resetPrograms()
{
    if (_cgVP != 0)
    {
        cgDestroyProgram(_cgVP);
    }

    if (_cgFP != 0)
    {
        cgDestroyProgram(_cgFP);
    }
}
