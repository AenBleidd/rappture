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
#ifndef MAIN_TF_WINDOW_H
#define MAIN_TF_WINDOW_H

#include "glui.h"
#include "ControlPoint.h"

extern int mainWindow;

extern int main_winx, main_winy;
extern int cm_winx, cm_winy;
extern int unitWidth, unitHeight;

class MainTransferFunctionWindow
{
public:
    MainTransferFunctionWindow();
    virtual ~MainTransferFunctionWindow();

    static void mainMouse(int button, int state, int x, int y);
    static void mainMotion(int x, int y);
    static void mainIdle();
    static void mainKeyboard(unsigned char key, int x, int y);
    static void mainDestroy();
    static void mainReshape(int x, int y);
    static void mainDisplay();
    static void mainInit();

    static void loadFile(char* fileName);
    static void saveFile(char* fileName);
    static void  changeState(int arg);

    static ControlPoint* selectPoint;
};

#endif
