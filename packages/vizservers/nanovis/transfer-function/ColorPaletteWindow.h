/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef COLOR_PALETTE_WINDOW_H
#define COLOR_PALETTE_WINODW_H

//ColorPalette Window to select color from
#include <glui.h>

#include "../Color.h"
#include "ControlPoint.h"

extern int cp_winx, cp_winy; //size of the colorPalette window
extern int cp_color_r, cp_color_g, cp_color_b; //color picked
extern GLUI* cp_glui; //glui handler

class ColorPalette
{
public:
    ColorPalette();
    virtual ~ColorPalette();

    static void cpMouse(int button, int state, int x, int y);
    static void cpMotion(int x, int y);
    static void cpIdle();
    static void cpKeyboard(unsigned char key, int x, int y);
    static void cpDestroy();
    static void cpReshape(int x, int y);
    static void cpDisplay();
    static void cpInit();
    static void cpSyncLive();

    static void createGLUIWidgets();
    static Color getColor(double x, double y);
    static void confirm(int arg);
    static void cmdHandler(int arg);
    static void openFile();
};

#endif
