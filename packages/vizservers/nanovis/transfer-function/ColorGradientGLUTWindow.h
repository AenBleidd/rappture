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
#ifndef COLOR_GRADIENT_GLUT_WINDOW_H
#define COLOR_GRADIENT_GLUT_WINDOW_H 

#include <glui.h>
#include "ColorGradient.h"
#include "ControlPoint.h"

extern int cm_winx, cm_winy;  //size of the subwindow
extern int cm_unitWidth;
extern ColorGradient *map;		//the color map
extern int cm_editState;
	
extern ControlPoint* cm_gvSelectedPoint; //current selected color controlpoint

///////Color Map Interpolation Result//////////////////////////////
#define MAP_NUM_OF_OUTPUT 256
extern int mapNumOfOutput;	//number of interpolations
extern float* mapOutput; //global color map interpolation output
///////////////////////////////////////////////////////////////////

extern GLUI *cm_glui;

class ColorGradientGLUTWindow  
{
public:
    ColorGradientGLUTWindow();
    virtual ~ColorGradientGLUTWindow();

    static void cmMouse(int button, int state, int x, int y);
    static void cmMotion(int x, int y);
    static void cmIdle();
    static void cmKeyboard(unsigned char key, int x, int y);
    static void cmDestroy();
    static void cmReshape(int x, int y);
    static void cmDisplay();

    static void cmInit(int main_win_x, int main_win_y);
    static bool SelectPoint(double x, double y);
    static void sortPoints();
    static ControlPoint* boundaryChecking();
    static void changeState(int arg);
    static void createGLUIWidgets();
    static void WriteControlPoints();
    static void printInterpolation();
};

#endif
