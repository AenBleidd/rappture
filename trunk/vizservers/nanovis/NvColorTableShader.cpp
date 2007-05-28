#include "NvColorTableShader.h"

NvColorTableShader::NvColorTableShader()
{
    init();
}

NvColorTableShader::~NvColorTableShader()
{
}

void NvColorTableShader::init()
{
    _cgFP= cgCreateProgramFromFile(g_context, CG_SOURCE,
                    "/opt/nanovis/lib/shaders/one_plane.cg", CG_PROFILE_FP30, "main", NULL);
    cgGLLoadProgram(_cgFP);

    _dataParam = cgGetNamedParameter(_cgFP, "data");
    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _renderParam = cgGetNamedParameter(_cgFP, "render_param");
}



