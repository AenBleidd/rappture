#include "NvVolQDVolumeShader.h"
#include <R2/R2FilePath.h>

NvVolQDVolumeShader::NvVolQDVolumeShader()
{
    init();
}

NvVolQDVolumeShader::~NvVolQDVolumeShader()
{
}

void NvVolQDVolumeShader::init()
{
    R2string path = R2FilePath::getInstance()->getPath("volqd_volume.cg");
    _cgFP = cgCreateProgramFromFile(g_context, CG_SOURCE,
            (const char*) path, 
            CG_PROFILE_FP30, "main", NULL);

    cgGLLoadProgram(_cgFP);

    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _volumeParam = cgGetNamedParameter(_cgFP, "vol_grid_s");
    _cellSizeParam = cgGetNamedParameter(_cgFP, "cell_size");
    _mviParam = cgGetNamedParameter(_cgFP, "ModelViewInv");
    _renderParam = cgGetNamedParameter(_cgFP, "renderParameters");
}
