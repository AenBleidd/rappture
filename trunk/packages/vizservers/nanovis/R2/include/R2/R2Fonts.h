/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __R2_FONTS_H__
#define __R2_FONTS_H__

#include <R2/R2.h>
#include <R2/R2Object.h>
#include <R2/R2string.h>
#include <vector>

class R2Fonts : public R2Object {

    struct R2FontAttributes {
        char *_fontName;
        R2int32 _textureWidth;
        R2int32 _textureHeight;
        R2int32 _fontHeight;
        R2uint32 _fontTextureID;
        R2uint32 _displayLists;
        
        struct R2CharInfo {
            R2float _left;
            R2float _right;
            R2float _top;
            R2float _bottom;
            R2bool _valid;
            R2float _width;
            
        };
        R2CharInfo _chars[256];
    };
    
    typedef std::vector<R2FontAttributes>  R2FontVector;

    R2FontVector        _fonts;
    
    /**
     * @brief current font index
     */
    R2int32 _fontIndex;
    
    /**
     * @brief screen width
     */
    R2int32 _screenWidth;
    
    /** 
     * @brief screen height
     */
    R2int32 _screenHeight;
    
private :
    ~R2Fonts();
    
    //private :
public :
    /**
     * @brief set projection to orthographic
     */
    void begin();
    
    /**
     * @brief reset projection matrix
     */
    void end();
    
    /** 
     * @brief initialize R2FontAttributes
     */
    void initializeFont(R2FontAttributes& attr);
    
    /**
     * @brief load font data
     */
    R2bool loadFont(const char* fontName, const char* fontFileName, R2FontAttributes& sFont);
    
public :
    /**
     * @brief constructor
     */
    R2Fonts();
    
    /**
     * @brief 
     */
    void addFont(const char* fontName, const char* fontFileName);
    
    /**
     * @brief
     */
    void setFont(const char* fontName);
    
    /**
     *
     */
    void draw(const char* pString, ...) const;
    
    /**
     *
     */
    void resize(R2int32 width, R2int32 height);
    
    /**
     * @brief return font height
     */
    R2int32 getFontHeight() const; 
    
};

inline R2int32 R2Fonts::getFontHeight() const
{
    return _fonts[_fontIndex]._fontHeight;
}

#endif // __R2_FONT_H__

