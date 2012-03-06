/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <R2/R2Fonts.h>
#include <R2/R2FilePath.h>
#include <fstream>
#include <stdarg.h>

// constants
const int c_nFileMagicHeader = 6666;

R2Fonts::R2Fonts() : 
    _fontIndex(-1), 
    _screenWidth(512), 
    _screenHeight(512)
{
}

R2Fonts::~R2Fonts()
{
    for (unsigned index = 0; index < _fonts.size(); ++index) {
        glDeleteLists(_fonts[index]._displayLists, 256);
        glDeleteTextures(1, &(_fonts[_fontIndex]. _fontTextureID));
    }
}

void 
R2Fonts::setFont(const char* fontName)
{
    if (fontName != NULL) {
	unsigned int i;
	for (i = 0; i < _fonts.size(); ++i) {
	    if (strcmp(_fonts[i]._fontName, fontName) == 0) {
		_fontIndex = i;
		break;
	    }
	}
    }
}


void 
R2Fonts::addFont(const char* fontName, const char* fontFileName)
{
    R2FontAttributes sFont;
    
    loadFont(fontName, fontFileName, sFont);
    initializeFont(sFont);
    _fonts.push_back(sFont);
}

void 
R2Fonts::draw(const char* pString, ...) const
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
R2Fonts::begin()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _fonts[_fontIndex]. _fontTextureID);
    
    glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    glLoadIdentity( );
    gluOrtho2D(0.0f, _screenWidth, _screenHeight, 0.0f);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix( );
    glLoadIdentity( );

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void 
R2Fonts::end()
{
    glBindTexture(GL_TEXTURE_2D, 0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix( );

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix( );

    glPopAttrib( );
}

void 
R2Fonts::initializeFont(R2FontAttributes& attr)
{
    attr._displayLists = glGenLists(256);

    R2int32 index;
    for (index = 0; index < 256; ++index) {
        R2FontAttributes::R2CharInfo& charInfo = attr._chars[index];
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

R2bool 
R2Fonts::loadFont(const char* fontName, const char* fontFileName, 
                  R2FontAttributes& sFont)
{
    R2bool bSuccess = false;

    const char *path = R2FilePath::getInstance()->getPath(fontFileName);
    if (path == NULL) {
	return false;
    }
    std::ifstream fsInput(path, std::ios::binary);
    if (fsInput) {
        sFont._fontName = new char [strlen(fontName)+1];
        strcpy(sFont._fontName, fontName);

        // make sure this file is the correct type by checking the header
        unsigned int uiFileId = 0;
        fsInput.read(reinterpret_cast<char*>(&uiFileId), sizeof(unsigned int));
        if (uiFileId == (unsigned int)c_nFileMagicHeader) {
            // read general font/texture dimensions
            unsigned int uiTextureWidth, uiTextureHeight, uiFontHeight;
            uiTextureWidth = uiTextureHeight = uiFontHeight = 0;
            fsInput.read(reinterpret_cast<char*>(&sFont._textureWidth), 
                         sizeof(unsigned int));
            fsInput.read(reinterpret_cast<char*>(&sFont._textureHeight), 
                         sizeof(unsigned int));
            fsInput.read(reinterpret_cast<char*>(&sFont._fontHeight), 
                         sizeof(unsigned int));

            // read dimensions for each charactor in 256-char ASCII chart
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
            unsigned char* pRawMap = new unsigned char[uiArea];
            fsInput.read(reinterpret_cast<char*>(pRawMap), uiArea);

            // we've only read the luminance values, but we need a luminance +
            // alpha buffer, so we make a new buffer and duplicate the
            // luminance values
            unsigned char* pTexMap = new unsigned char[2 * uiArea];
            unsigned char* pMap = pTexMap;
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

        fsInput.close( );
    }
    delete [] path;
    return bSuccess;
}

void 
R2Fonts::resize(R2int32 width, R2int32 height)
{
    _screenWidth = width;
    _screenHeight = height;
}

