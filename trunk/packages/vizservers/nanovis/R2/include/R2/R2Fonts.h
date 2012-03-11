/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_FONTS_H
#define R2_FONTS_H

#include <vector>

#include <R2/R2.h>
#include <R2/R2Object.h>
#include <R2/R2string.h>

class R2Fonts : public R2Object
{
    struct R2FontAttributes {
        char *_fontName;
        int _textureWidth;
        int _textureHeight;
        int _fontHeight;
        unsigned int _fontTextureID;
        unsigned int _displayLists;

        struct R2CharInfo {
            float _left;
            float _right;
            float _top;
            float _bottom;
            bool _valid;
            float _width;
        };
        R2CharInfo _chars[256];
    };
    
    typedef std::vector<R2FontAttributes>  R2FontVector;

public:
    R2Fonts();

    /// set projection to orthographic
    void begin();

    /// reset projection matrix
    void end();

    /// initialize R2FontAttributes
    void initializeFont(R2FontAttributes& attr);

    /**
     * @brief load font data
     */
    bool loadFont(const char *fontName, const char *fontFileName, R2FontAttributes& sFont);

    void addFont(const char *fontName, const char *fontFileName);

    void setFont(const char *fontName);

    void draw(const char *pString, ...) const;

    void resize(int width, int height);

    /// return font height
    int getFontHeight() const;

private:
    ~R2Fonts();

    R2FontVector _fonts;
    /// current font index
    int _fontIndex;
    // screen width
    int _screenWidth;
    /// screen height
    int _screenHeight;
};

inline int R2Fonts::getFontHeight() const
{
    return _fonts[_fontIndex]._fontHeight;
}

#endif

