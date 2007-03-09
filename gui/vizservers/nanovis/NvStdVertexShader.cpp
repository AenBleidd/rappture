#include <R2/R2FilePath.h>
#include "NvStdVertexShader.h"

NvStdVertexShader::NvStdVertexShader()
{
    init();
}

NvStdVertexShader::~NvStdVertexShader()
{
}

void NvStdVertexShader::init()
{
    R2string path = R2FilePath::getInstance()->getPath("vertex_std.cg");
    _cgVP= cgCreateProgramFromFile(g_context, CG_SOURCE,
                (const char*) path, CG_PROFILE_VP30, "main", NULL);
    cgGLLoadProgram(_cgVP);

    _mvp_vert_std_param = cgGetNamedParameter(_cgVP, "modelViewProjMatrix");
    _mvi_vert_std_param = cgGetNamedParameter(_cgVP, "modelViewInv");
}

