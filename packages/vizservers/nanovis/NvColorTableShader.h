/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __NV_COLORTABLE_SHADER_H__
#define __NV_COLORTABLE_SHADER_H__

#include "Texture2D.h"
#include "TransferFunction.h"
#include "NvShader.h"

class NvColorTableShader : public NvShader {
    CGparameter _dataParam;
    CGparameter _tfParam;
    CGparameter _renderParam;

public :
    NvColorTableShader();
    ~NvColorTableShader();

private :
    void init();
public :
    void bind(Texture2D* plane, TransferFunction* tf);
    void unbind();
};

inline void NvColorTableShader::bind(Texture2D* plane, TransferFunction* tf)
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
