#include <R2/R2FilePath.h>
#include "NvColorTableShader.h"
#include <Trace.h>

NvColorTableShader::NvColorTableShader()
{
    init();
}

NvColorTableShader::~NvColorTableShader()
{
}

void NvColorTableShader::init()
{
    R2string path = R2FilePath::getInstance()->getPath("one_plane.cg");
    if (path.getLength() == 0)
    {
        Trace("%s is not found", (const char*) path); 
    }
    
    _cgFP = cgCreateProgramFromFile(g_context, CG_SOURCE,
            path, CG_PROFILE_FP30, "main", NULL);
    cgGLLoadProgram(_cgFP);

    _dataParam = cgGetNamedParameter(_cgFP, "data");
    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _renderParam = cgGetNamedParameter(_cgFP, "render_param");
}



