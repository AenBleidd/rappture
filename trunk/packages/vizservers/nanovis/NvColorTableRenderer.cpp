/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdlib.h>

#include <R2/R2Fonts.h>

#include "NvColorTableRenderer.h"

NvColorTableRenderer::NvColorTableRenderer() :
    _fonts(NULL),
    _shader(new NvColorTableShader())
{
}

NvColorTableRenderer::~NvColorTableRenderer()
{
    delete _shader;
}

void NvColorTableRenderer::render(int width, int height,
                                  Texture2D *texture, TransferFunction *tf,
                                  double rangeMin, double rangeMax)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    //glColor3f(1., 1., 1.);         //MUST HAVE THIS LINE!!!
    _shader->bind(texture, tf);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(30, 30);
    glTexCoord2f(1, 0); glVertex2f(width - 30, 30);
    glTexCoord2f(1, 1); glVertex2f(width - 30, 60);
    glTexCoord2f(0, 1); glVertex2f(30, 60);
    glEnd();

    _shader->unbind();

    if (_fonts) {
        _fonts->resize(width, height);
        _fonts->begin();

        glPushMatrix();
            glTranslatef(width - 110, 5, 0.0f);
            _fonts->draw("Quantum dot lab - www.nanohub.org");
        glPopMatrix();

        glPushMatrix();
            glTranslatef(30, height - 25, 0.0f);
            _fonts->draw("%.08lf", rangeMin);
        glPopMatrix();

        glPushMatrix();
            glTranslatef(width - 110, height - 25, 0.0f);
            _fonts->draw("%.08lf", rangeMax);
        glPopMatrix();

        _fonts->end();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
