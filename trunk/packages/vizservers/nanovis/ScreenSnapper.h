/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * ScreenSnapper.h: ScreenSnapper class. It captures the render result
 * 			and stores it in an array of chars or floats 
 * 			depending on chosen format.
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef SCREEN_SNAPPER_H 
#define SCREEN_SNAPPER_H

#include <stdio.h>

#include <GL/glew.h>

class ScreenSnapper 
{
public:
    ScreenSnapper(int width, int height, GLuint type, int channel_per_pixel);
    ~ScreenSnapper();

    /// set every byte in the data array to c
    void reset(char c);

    void capture();

    void print();

private:
    int _width;  ///< width of the screen
    int _height;	///< height of the screen
    GLuint _dataType;	///< GL_FLOAT or GL_UNSIGNED_BYTE
    int _numChannelsPerPixel; ///< RGB(3) or RGBA(4)

    /**
     * storage array for the captured image. This array is "flat".
     * It stores pixels in the order from lower-left corner to 
     * upper-right corner.
     * [rgb][rgb][rgb]... or [rgba][rgba][rgba]...
     */
    void *_data;
};

#endif
