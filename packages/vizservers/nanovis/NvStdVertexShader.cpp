/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include "global.h"
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
    _cgVP = LoadCgSourceProgram(g_context, "vertex_std.cg", CG_PROFILE_VP30, 
	"main");
    _mvp_vert_std_param = cgGetNamedParameter(_cgVP, "modelViewProjMatrix");
    _mvi_vert_std_param = cgGetNamedParameter(_cgVP, "modelViewInv");
}

