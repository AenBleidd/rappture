/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_COLORTABLE_SHADER_H
#define NV_COLORTABLE_SHADER_H

#include <Cg/cg.h>

#include "Texture2D.h"
#include "TransferFunction.h"
#include "NvShader.h"

class NvColorTableShader : public NvShader
{
public:
    NvColorTableShader();

    ~NvColorTableShader();

    void bind(Texture2D *plane, TransferFunction *tf);

    void unbind();

private :
    void init();

    CGparameter _dataParam;
    CGparameter _tfParam;
    CGparameter _renderParam;
};

inline void NvColorTableShader::bind(Texture2D *plane, TransferFunction *tf)
{
    cgGLSetTextureParameter(_dataParam, plane->id);
    cgGLSetTextureParameter(_tfParam, tf->id());
    cgGLEnableTextureParameter(_dataParam);
    cgGLEnableTextureParameter(_tfParam);
    cgGLSetParameter4f(_renderParam, 0., 0., 0., 0.);

    cgGLBindProgram(_cgFP);
    cgGLEnableProfile(CG_PROFILE_FP30);
}

inline void NvColorTableShader::unbind()
{
    cgGLDisableProfile(CG_PROFILE_FP30);
    cgGLDisableTextureParameter(_dataParam);
    cgGLDisableTextureParameter(_tfParam);
}

#endif 
