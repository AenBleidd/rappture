/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <vrmath/Vector4f.h>

#include "StdVertexShader.h"

using namespace vrmath;
using namespace nv;

StdVertexShader::StdVertexShader()
{
    init();
}

StdVertexShader::~StdVertexShader()
{
}

void StdVertexShader::init()
{
    loadVertexProgram("vertex_std.cg");
}

void StdVertexShader::bind(const Vector4f& objPlaneS,
                           const Vector4f& objPlaneT,
                           const Vector4f& objPlaneR,
                           float *mvp, float *mvInv)
{
    if (mvp != NULL) {
        setVPMatrixParameterf("modelViewProjMatrix", mvp);
    } else {
        setGLStateMatrixVPParameter("modelViewProjMatrix",
                                    MODELVIEW_PROJECTION_MATRIX);
    }
    if (mvInv != NULL) {
        setVPMatrixParameterf("modelViewInv", mvInv);
    } else {
        setGLStateMatrixVPParameter("modelViewInv",
                                    MODELVIEW_MATRIX, MATRIX_INVERSE);
    }

    setVPParameter4f("light0Position", 1, 1, 1, 1);
    setVPParameter4f("objPlaneS",
                     objPlaneS.x, objPlaneS.y, objPlaneS.z, objPlaneS.w);
    setVPParameter4f("objPlaneT",
                     objPlaneT.x, objPlaneT.y, objPlaneT.z, objPlaneT.w);
    setVPParameter4f("objPlaneR",
                     objPlaneR.x, objPlaneR.y, objPlaneR.z, objPlaneR.w);

    Shader::bind();
}
