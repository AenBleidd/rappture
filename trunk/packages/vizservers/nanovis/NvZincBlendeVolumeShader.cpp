
#include <R2/R2string.h>
#include <R2/R2FilePath.h>
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
    R2string path = R2FilePath::getInstance()->getPath("zincblende_volume.cg");
    _cgFP = cgCreateProgramFromFile(g_context, CG_SOURCE, 
        (const char*) path, CG_PROFILE_FP30, "main", NULL);

    cgGLLoadProgram(_cgFP);

    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _volumeAParam = cgGetNamedParameter(_cgFP, "volumeA");
    _volumeBParam = cgGetNamedParameter(_cgFP, "volumeB");
    _cellSizeParam = cgGetNamedParameter(_cgFP, "cellSize");
    _mviParam = cgGetNamedParameter(_cgFP, "modelViewInv");
    _renderParam = cgGetNamedParameter(_cgFP, "renderParameters");
    _option_one_volume_param = cgGetNamedParameter(_cgFP, "options");
}

