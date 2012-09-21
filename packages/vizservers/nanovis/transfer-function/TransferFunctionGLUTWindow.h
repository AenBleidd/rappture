/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef TRANSFER_FUNCTION_GLUT_WINDOW_H
#define TRANSFER_FUNCTION_GLUT_WINDOW_H 

#include <stdlib.h>
#include <time.h>
#include <float.h>

#include "ControlPoint.h"

////////////////////Interpolation Result/////////////////////////////////////////
#define NUM_OF_OUTPUT 256	//define
extern int numOfOutput;		//number of interpolations in the output array
extern float* output;		//result array of interpolation of control points
/////////////////////////////////////////////////////////////////////////////////

extern int tf_winx, tf_winy;			//window size
extern int tf_unitWidth;			//unit width
extern bool tf_gvIsDragging;			//is dragging a control point
extern ControlPoint *tf_gvSelectedPoint;	//selected controlpoint
extern float tfMaximum;
extern ControlPoint* tf_pointList;		//list of controlpoint
extern int tf_numOfPoints;			//number of controlpoints
extern int transferFuctionWindow;		//global winid
extern int tf_pointEditState;			//pointEditState
                                                  //if ==1, add point
                                                  //if ==2, remove point
struct Histogram
{
    Histogram()
    {
        range =256;
        min = FLT_MAX;
        max = FLT_MIN;
        count = (unsigned long*) malloc(sizeof(unsigned long)*(range));

        int i=0;
        for (i=0; i<range; i++){
            count[i]=0;
        }
    };

    int range;
    unsigned long* count;
    float min;
    float max;
};

extern Histogram Hist[4]; //s, p, d, ss histograms

class TransferFunctionGLUTWindow  
{
public:
    TransferFunctionGLUTWindow();
    virtual ~TransferFunctionGLUTWindow();

    void static tfInit(int main_window_x, int main_window_y);
    void static WriteControlPoints();
    void static ReadControlPoints();

    void static tfReshape(int x, int y);
    void static tfKeyboard(unsigned char key, int x, int y);
    void static tfIdle();
    void static tfMouse(int button, int state, int x, int y);
    void static tfMotion(int x, int y);
    void static tfDisplay();	

    bool static SelectPoint(double x, double y);

    void static sortPoints();				//sort all the points by there x coordinates
    static ControlPoint* addPoint(double x, double y);	//add control point
    void static removePoint(void* ptr);			//remove control point
    void static scalePointsY(int offset);		//scale point X
    void static scalePointsX();				//scale point Y
    static ControlPoint* boundaryChecking();		//point boundary checking
    void static plotHist();				//draw Histogram in transformation window
    static void cleanUpPoints();			//remove all points but the two initial points

    void static printPoints();				//debugging: print out all points
    void static printInterpolation();			//debugging: print interpolation	
    void static dumpHist();				//dump histogram
    void static readHist();				//read histogram
    void static printHist();

};


#endif
