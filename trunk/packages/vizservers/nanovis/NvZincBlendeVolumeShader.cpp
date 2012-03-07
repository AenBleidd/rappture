/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <string.h>
#include "global.h"
#include "NvZincBlendeVolumeShader.h"


NvZincBlendeVolumeShader::NvZincBlendeVolumeShader()
{
    init();
}

NvZincBlendeVolumeShader::~NvZincBlendeVolumeShader()
{
}

void NvZincBlendeVolumeShader::init()
{
    _cgFP = LoadCgSourceProgram(g_context, "zincblende_volume.cg", 
	CG_PROFILE_FP30, "main");
    _tfParam = cgGetNamedParameter(_cgFP, "tf");
    _volumeAParam = cgGetNamedParameter(_cgFP, "volumeA");
    _volumeBParam = cgGetNamedParameter(_cgFP, "volumeB");
    _cellSizeParam = cgGetNamedParameter(_cgFP, "cellSize");
    _mviParam = cgGetNamedParameter(_cgFP, "modelViewInv");
    _renderParam = cgGetNamedParameter(_cgFP, "renderParameters");
    _option_one_volume_param = cgGetNamedParameter(_cgFP, "options");
}

