
#include "Nv.h"
#include "NvZincBlendeVolumeShader.h"

#include <string.h>

NvZincBlendeVolumeShader::NvZincBlendeVolumeShader()
{
    init();
}

NvZincBlendeVolumeShader::~NvZincBlendeVolumeShader()
{
}

void NvZincBlendeVolumeShader::init()
{
    //_cgFP = cgCreateProgramFromFile(g_context, CG_SOURCE, 
    //    "/opt/nanovis/lib/shaders/zincblende_volume.cg", CG_PROFILE_FP30, "main", NULL);
    _cgFP = cgCreateProgramFromFile(g_context, CG_SOURCE, 
        "/home/nanohub/vrinside/rappture/gui/vizservers/nanovis/shaders/zincblende_volume.cg", CG_PROFILE_FP30, "main", NULL);

    cgGLLoadProgram(_cgFP);

    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _volumeAParam = cgGetNamedParameter(_cgFP, "volumeA");
    _volumeBParam = cgGetNamedParameter(_cgFP, "volumeB");
    _cellSizeParam = cgGetNamedParameter(_cgFP, "cellSize");
    _mviParam = cgGetNamedParameter(_cgFP, "modelViewInv");
    _renderParam = cgGetNamedParameter(_cgFP, "renderParameters");
}

