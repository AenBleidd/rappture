#include "NvVolQDVolumeShader.h"

NvVolQDVolumeShader::NvVolQDVolumeShader()
{
    init();
}

NvVolQDVolumeShader::~NvVolQDVolumeShader()
{
}

void NvVolQDVolumeShader::init()
{
    _cgFP = cgCreateProgramFromFile(g_context, CG_SOURCE,
            "/home/nanohub/vrinside/rappture/gui/vizservers/nanovis/shaders/volqd_volume.cg", 
            CG_PROFILE_FP30, "main", NULL);

    cgGLLoadProgram(_cgFP);

    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _volumeParam = cgGetNamedParameter(_cgFP, "vol_grid_s");
    _cellSizeParam = cgGetNamedParameter(_cgFP, "cell_size");
    _mviParam = cgGetNamedParameter(_cgFP, "ModelViewInv");
    _renderParam = cgGetNamedParameter(_cgFP, "renderParameters");
}
