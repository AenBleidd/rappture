/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_COLORTABLE_RENDERER_H
#define NV_COLORTABLE_RENDERER_H

#include <util/Fonts.h>

#include "Texture2D.h"
#include "TransferFunction.h"
#include "NvColorTableShader.h"

class NvColorTableRenderer
{
public :
    NvColorTableRenderer();
    ~NvColorTableRenderer();

    void render(int width, int height,
                Texture2D *texture, TransferFunction *tf,
                double rangeMin, double rangeMax);

    void setFonts(nv::util::Fonts *fonts)
    {
        _fonts = fonts;
    }

private:
    nv::util::Fonts *_fonts;
    NvColorTableShader *_shader;
};

#endif 
