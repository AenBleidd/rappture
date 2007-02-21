#include "NvShader.h"

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

