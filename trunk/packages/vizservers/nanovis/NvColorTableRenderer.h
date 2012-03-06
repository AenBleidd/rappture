/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __NV_COLORTABLE_RENDERER_H__
#define __NV_COLORTABLE_RENDERER_H__

#include "Texture2D.h"
#include "TransferFunction.h"
#include "NvColorTableShader.h"

#include <R2/R2Fonts.h>

class NvColorTableRenderer {
    NvColorTableShader* _shader;
    R2Fonts* _fonts;
public :
    NvColorTableRenderer();
    ~NvColorTableRenderer();

public :
    void render(int width, int height, Texture2D* texture, TransferFunction* tf, double rangeMin, double rageMax);
    void setFonts(R2Fonts* fonts);
};

inline void NvColorTableRenderer::setFonts(R2Fonts* fonts)
{
    _fonts = fonts;
}

#endif 

