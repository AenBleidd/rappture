/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 */

#include <stdarg.h>
#include <string.h>

#include <fstream>

#include <GL/glew.h>

#include "Fonts.h"
#include "FilePath.h"

using namespace nv::util;

// constants
const int c_nFileMagicHeader = 6666;

Fonts::Fonts() : 
    _fontIndex(-1), 
    _screenWidth(512), 
    _screenHeight(512)
{
}

Fonts::~Fonts()
{
    for (unsigned index = 0; index < _fonts.size(); ++index) {
        glDeleteLists(_fonts[index]._displayLists, 256);
        glDeleteTextures(1, &(_fonts[_fontIndex]. _fontTextureID));
    }
}

void 
Fonts::setFont(const char *fontName)
{
    if (fontName != NULL) {
        unsigned int i;
        for (i = 0; i < _fonts.size(); ++i) {
            if (strcmp(_fonts[i]._fontName.c_str(), fontName) == 0) {
                _fontIndex = i;
                break;
            }
        }
    }
}

void 
Fonts::addFont(const char *fontName, const char *fontFileName)
{
    FontAttributes sFont;
    
    loadFont(fontName, fontFileName, sFont);
    initializeFont(sFont);
    _fonts.push_back(sFont);
}

void 
Fonts::draw(const char *pString, ...) const
{
    va_list vlArgs;
    char szVargsBuffer[1024];

    va_start(vlArgs, pString);
    vsprintf(szVargsBuffer, pString, vlArgs);

    if (_fontIndex != -1) {
        int length = strlen(szVargsBuffer);

        glListBase(_fonts[_fontIndex]._displayLists);
        glCallLists(length, GL_UNSIGNED_BYTE, 
                    reinterpret_cast<const GLvoid*>(szVargsBuffer));
    }
}

void 
Fonts::begin()
{
    glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _fonts[_fontIndex]._fontTextureID);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    gluOrtho2D(0.0f, _screenWidth, _screenHeight, 0.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void 
Fonts::end()
{
    glBindTexture(GL_TEXTURE_2D, 0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

void 
Fonts::initializeFont(FontAttributes& attr)
{
    attr._displayLists = glGenLists(256);

    int index;
    for (index = 0; index < 256; ++index) {
        FontAttributes::CharInfo& charInfo = attr._chars[index];
        glNewList(attr._displayLists + index, GL_COMPILE);
        if (charInfo._valid) {
            glBegin(GL_TRIANGLE_STRIP);
            
            glTexCoord2f(charInfo._left, charInfo._top); 
            glVertex2i(0, 0);
            
            glTexCoord2f(charInfo._left, charInfo._bottom);
            glVertex2i(0, (GLint)attr._fontHeight);
            
            glTexCoord2f(charInfo._right, charInfo._top);
            glVertex2i((GLint)charInfo._width, 0);
            
            glTexCoord2f(charInfo._right,  charInfo._bottom);
            glVertex2i((GLint)charInfo._width, (GLint)attr._fontHeight);
            
            glEnd();
            glTranslatef(charInfo._width, 0.0f, 0.0f);
        }
        glEndList();
    }
}

bool 
Fonts::loadFont(const char *fontName, const char *fontFileName, 
                FontAttributes& sFont)
{
    bool bSuccess = false;

    std::string path = FilePath::getInstance()->getPath(fontFileName);
    if (path.empty()) {
        return false;
    }
    std::ifstream fsInput(path.c_str(), std::ios::binary);
    if (fsInput) {
        sFont._fontName = fontName;

        // make sure this file is the correct type by checking the header
        unsigned int uiFileId = 0;
        fsInput.read(reinterpret_cast<char*>(&uiFileId), sizeof(unsigned int));
        if (uiFileId == (unsigned int)c_nFileMagicHeader) {
            // read general font/texture dimensions
            fsInput.read(reinterpret_cast<char*>(&sFont._textureWidth), 
                         sizeof(unsigned int));
            fsInput.read(reinterpret_cast<char*>(&sFont._textureHeight), 
                         sizeof(unsigned int));
            fsInput.read(reinterpret_cast<char*>(&sFont._fontHeight), 
                         sizeof(unsigned int));

            // read dimensions for each character in 256-char ASCII chart
            for (int i = 0; i < 256; ++i) {
                unsigned int uiSize = 0;

                // top
                fsInput.read(reinterpret_cast<char*>(&uiSize), 
                             sizeof(unsigned int));
                sFont._chars[i]._top = static_cast<float>(uiSize) / sFont._textureHeight;
                // left
                fsInput.read(reinterpret_cast<char*>(&uiSize), sizeof(unsigned int));
                sFont._chars[i]._left = static_cast<float>(uiSize) / sFont._textureWidth;
                // bottom
                fsInput.read(reinterpret_cast<char*>(&uiSize), sizeof(unsigned int));
                sFont._chars[i]._bottom = static_cast<float>(uiSize) / sFont._textureHeight;
                // right
                fsInput.read(reinterpret_cast<char*>(&uiSize), sizeof(unsigned int));
                sFont._chars[i]._right = static_cast<float>(uiSize) / sFont._textureWidth;
                // enabled
                fsInput.read(reinterpret_cast<char*>(&uiSize), sizeof(unsigned int));
                sFont._chars[i]._valid = (uiSize != 0);
                // width factor
                float fWidthFactor = 1.0f;
                fsInput.read(reinterpret_cast<char*>(&fWidthFactor), sizeof(float));
                sFont._chars[i]._width = fWidthFactor * sFont._fontHeight;
            }
        }

        // allocate and read the texture map
        if (!fsInput.eof() && !fsInput.fail()) {
            unsigned int uiArea = sFont._textureWidth * sFont._textureHeight;
            unsigned char *pRawMap = new unsigned char[uiArea];
            fsInput.read(reinterpret_cast<char *>(pRawMap), uiArea);

            // we've only read the luminance values, but we need a luminance +
            // alpha buffer, so we make a new buffer and duplicate the
            // luminance values
            unsigned char *pTexMap = new unsigned char[2 * uiArea];
            unsigned char *pMap = pTexMap;
            for (unsigned int i = 0; i < uiArea; ++i) {
                *pMap++ = pRawMap[i];
                *pMap++ = pRawMap[i];
            }
            delete[] pRawMap;
            pRawMap = NULL;

            // make texture map out of new buffer
            glGenTextures(1, &sFont._fontTextureID);
            glBindTexture(GL_TEXTURE_2D, sFont._fontTextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 
                         sFont._textureWidth, sFont._textureHeight, 0, 
                         GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pTexMap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glBindTexture(GL_TEXTURE_2D, 0);

            delete[] pTexMap;
            pTexMap = NULL;

            bSuccess = true;
        }

        fsInput.close();
    }
    return bSuccess;
}

void 
Fonts::resize(int width, int height)
{
    _screenWidth = width;
    _screenHeight = height;
}

