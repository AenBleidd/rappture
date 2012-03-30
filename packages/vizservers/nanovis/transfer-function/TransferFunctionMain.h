/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <GL/glut.h>
#include <glui.h>

#ifndef TF_MAIN_H
#define TF_MAIN_H

extern bool gvIsDragging;
extern int colorPaletteWindow;		//glut windowID: color palette
extern int mainWindow;			//glut windowID: mainwindow 
extern int colorMapWindow;		//glut windowID: colorMap subwindow
extern int transferFunctionWindow;	//glut windowID: transforFunction 

extern float color_table[256][4];
extern float control_point_scale;

#endif
