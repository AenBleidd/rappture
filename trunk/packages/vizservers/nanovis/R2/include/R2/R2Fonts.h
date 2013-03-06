/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_UTIL_FONTS_H
#define NV_UTIL_FONTS_H

#include <vector>
#include <string>

#include <R2/R2Object.h>

namespace nv {
namespace util {

class Fonts : public Object
{
    struct FontAttributes {
        std::string _fontName;
        int _textureWidth;
        int _textureHeight;
        int _fontHeight;
        unsigned int _fontTextureID;
        unsigned int _displayLists;

        struct CharInfo {
            float _left;
            float _right;
            float _top;
            float _bottom;
            bool _valid;
            float _width;
        };
        CharInfo _chars[256];
    };

    typedef std::vector<FontAttributes>  FontVector;

public:
    Fonts();

    /// set projection to orthographic
    void begin();

    /// reset projection matrix
    void end();

    /// initialize FontAttributes
    void initializeFont(FontAttributes& attr);

    /**
     * @brief load font data
     */
    bool loadFont(const char *fontName, const char *fontFileName, FontAttributes& sFont);

    void addFont(const char *fontName, const char *fontFileName);

    void setFont(const char *fontName);

    void draw(const char *pString, ...) const;

    void resize(int width, int height);

    /// return font height
    int getFontHeight() const;

private:
    ~Fonts();

    FontVector _fonts;
    /// current font index
    int _fontIndex;
    // screen width
    int _screenWidth;
    /// screen height
    int _screenHeight;
};

inline int Fonts::getFontHeight() const
{
    return _fonts[_fontIndex]._fontHeight;
}

}
}

#endif

